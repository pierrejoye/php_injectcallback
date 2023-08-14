<?php

class MyClass {
    function foo($b) {
        $a = $b*32;
        if ($a > 3) {
            echo "a > 3\n";
        }
        echo "method foo called\n";
        return $a;
    }

    function bar() {
        $a = 23*32;
        if ($a > 3) {
            echo "asdksaj
            asdjasdl\n";
        }
        return $a;
    }
}

function stacktracer_call() {
    print_r(debug_backtrace());
}

$a = new MyClass();
$a->foo(3);
/* require_once __DIR__ ."/../php-ast/util.php";
$ast = \ast\parse_code(file_get_contents(__FILE__), $version=50);
var_dump($ast);
echo ast_dump($ast), "\n";
 */