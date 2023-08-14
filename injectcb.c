/* injectcb extension for PHP */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "php.h"
#include "ext/standard/info.h"
#include "main/SAPI.h"
#include "php_injectcb.h"
#include <string.h>

#define INJECTCB_G(e) php_injectcb_globals.e

typedef struct _php_injectcb_globals_t {
    HashTable *ast_to_clean;
    char class_name[1024];
    char method_name[512];

} php_injectcb_globals_t;

ZEND_TLS php_injectcb_globals_t php_injectcb_globals;

ZEND_API void (*original_ast_process_function)(zend_ast *ast);

static zend_ast *create_ast_call(const char *name, zend_ulong name_len)
{
    zend_ast *call;
    zend_ast_zval *name_var;
    zend_ast_list *arg_list;

    name_var = emalloc(sizeof(zend_ast_zval));
    name_var->kind = ZEND_AST_ZVAL;
    name_var->attr = 0;
    ZVAL_STRING(&name_var->val, name);
    name_var->val.u2.lineno = 0;
    zend_hash_next_index_insert_ptr(INJECTCB_G(ast_to_clean), name_var);

    arg_list = emalloc(sizeof(zend_ast_list) + sizeof(zend_ast*));
    arg_list->kind = ZEND_AST_ARG_LIST;
    arg_list->lineno = 0;
    arg_list->children = 0;
    zend_hash_next_index_insert_ptr(INJECTCB_G(ast_to_clean), arg_list);

    call = emalloc(sizeof(zend_ast) + sizeof(zend_ast*));
    call->kind = ZEND_AST_CALL;
    call->lineno = 0;
    call->child[0] = (zend_ast*)name_var;
    call->child[1] = (zend_ast*)arg_list;
    zend_hash_next_index_insert_ptr(INJECTCB_G(ast_to_clean), call);

    return call;
}

static zend_ast_list *inject_ast(zend_ast *original)
{
    zend_ast *try;
    zend_ast_list *new_list3, *new_list2, *finally, *empty_catches;
    zend_ast_list *new_list;

    new_list = emalloc(sizeof(zend_ast_list) + sizeof(zend_ast*));
    new_list->kind = ZEND_AST_STMT_LIST;
    new_list->lineno = 0;
    new_list->children = 2;
    new_list->child[0] = create_ast_call("stacktracer_call", sizeof("stacktracer_call"));
    new_list->child[1] = original;

    zend_hash_next_index_insert_ptr(INJECTCB_G(ast_to_clean), new_list);

    return (zend_ast_list *)new_list;
}

void imjectcb_ast_process_method(zend_ast *ast, zend_string *namespace, zend_string *class_name) {
    zend_ast_decl *decl;
    zend_ast *child;
    zend_string *method_name;

    if (ast->kind != ZEND_AST_METHOD) {
        return;
    }

    decl = (zend_ast_decl *)ast;

    method_name = decl->name;

    if (INJECTCB_G(method_name) == NULL) {
        return;
    }
    if (strcmp(ZSTR_VAL(method_name), INJECTCB_G(method_name)) != 0) {
        php_printf("%s not requested\n", ZSTR_VAL(method_name));
        return;
    }
    decl->child[2] = (zend_ast *)inject_ast(decl->child[2]);
}

void imjectcb_ast_process_class(zend_ast *ast, zend_string *namespace) {
    zend_string *class_name;
    zend_ast_list *list;
    zend_ast *child;
    zend_ast_decl *decl;
    const char * found;
    int i = 0;

    if (ast->kind != ZEND_AST_CLASS) {
        return;
    }

    decl = (zend_ast_decl *)ast;

    class_name = decl->name;
    if (INJECTCB_G(class_name) == NULL) {
        php_printf("No injection request defined\n");
        return;
    } 
    if (strcmp(ZSTR_VAL(class_name), INJECTCB_G(class_name)) != 0) {
        php_printf("%s not requested\n", ZSTR_VAL(class_name));
        return;
    }

    php_printf("%s requested %s\n", ZSTR_VAL(class_name),  INJECTCB_G(class_name));
    // TODO: Add dynamic check here
    if (1) {
        // continue with class processing, it has at least one method that is hooked
        list = zend_ast_get_list(decl->child[2]);

        for (i = 0; i < list->children; i++) {
            child = list->child[i];

            // check for methods
            if (child->kind == ZEND_AST_METHOD) {
                imjectcb_ast_process_method(child, namespace, class_name);
            }
        }
    }
}

void imjectcb_ast_process_file(zend_ast *ast) {
    int i = 0;
    zend_ast_list *list;
    zend_ast *child;
    zend_string *namespace;

    if (ast->kind != ZEND_AST_STMT_LIST) {
        return;
    }

    list = zend_ast_get_list(ast);

    for (i = 0; i < list->children; i++) {
        child = list->child[i];

        switch (child->kind) {
            case ZEND_AST_NAMESPACE:
                namespace = zend_ast_get_str(child->child[0]);
                break;

            case ZEND_AST_CLASS:
                imjectcb_ast_process_class(child, namespace);
                break;

            case ZEND_AST_FUNC_DECL:
                //imjectcb_ast_process_function(child, namespace);
                break;
        }
    }

    php_printf("%s\n", ZSTR_VAL(zend_ast_export("", ast, "")));
}

void imjectcb_ast_process(zend_ast *ast)
{
    imjectcb_ast_process_file(ast);

    /* call the original zend_ast_process function if one was set */
    if (original_ast_process_function) {
        original_ast_process_function(ast);
    }
}

static void set_class_method_filter() {
    char* filter_method = getenv("INJECTCB_METHOD");
    const zend_ulong  filter_method_len = filter_method ? strlen(filter_method) : 0;
    const char *found = NULL;

    if (!filter_method) {
        return;
    }

    // php_printf("Method set: %s\n", filter_method);
    found = (char*)php_memnstr(filter_method, "::", sizeof("::") -1, filter_method + filter_method_len);
    if (!found) {
        INJECTCB_G(class_name)[0] = 0;
        INJECTCB_G(method_name)[0] = 0;
        return;
    }

    // No FQNS implemented yet
/*     INJECTCB_G(class_name) = emalloc((found - filter_method) + 2);
    INJECTCB_G(method_name) = emalloc((filter_method_len - (found - filter_method))); */
    snprintf((char *) INJECTCB_G(class_name), found - filter_method + 1, "%s", (char *)filter_method);
    snprintf((char *) INJECTCB_G(method_name), strlen(found), "%s", (char *)(found+2));

/*     php_printf("len class: %i %s\n", (int)(found - filter_method), filter_method );
    php_printf("len method: %i %s\n", (int)(found - filter_method - 2) - 2, found +2); */

    php_printf("Class: %s\n", INJECTCB_G(class_name));
    php_printf("Method: %s\n", INJECTCB_G(method_name));
}

PHP_MINIT_FUNCTION(injectcb)
{
    char *filter_method;
    original_ast_process_function = zend_ast_process;
    zend_ast_process = imjectcb_ast_process;
    set_class_method_filter();
    return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(injectcb)
{
    return SUCCESS;
}

static void ast_to_clean_dtor(zval *zv)
{
    zend_ast *ast = (zend_ast *)Z_PTR_P(zv);
    efree(Z_PTR_P(zv));
}

PHP_RINIT_FUNCTION(injectcb)
{
#if defined(ZTS) && defined(COMPILE_DL_INJECTCB)
    ZEND_TSRMLS_CACHE_UPDATE();
#endif

    ALLOC_HASHTABLE(INJECTCB_G(ast_to_clean));
    zend_hash_init(INJECTCB_G(ast_to_clean), 8, NULL, ast_to_clean_dtor, 0);

    return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(injectcb)
{
    zend_hash_destroy(INJECTCB_G(ast_to_clean));
    FREE_HASHTABLE(INJECTCB_G(ast_to_clean));

    return SUCCESS;
}

PHP_MINFO_FUNCTION(injectcb)
{
    php_info_print_table_start();
    php_info_print_table_header(2, "injectcb support", "enabled");
    php_info_print_table_end();
}

zend_module_entry injectcb_module_entry = {
    STANDARD_MODULE_HEADER,
    "injectcb",
    NULL,
    PHP_MINIT(injectcb),
    PHP_MSHUTDOWN(injectcb),
    PHP_RINIT(injectcb),
    PHP_RSHUTDOWN(injectcb),
    PHP_MINFO(injectcb),
    PHP_INJECTCB_VERSION,
    STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_INJECTCB
# ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
# endif
ZEND_GET_MODULE(injectcb)
#endif
