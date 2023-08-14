# Simple function injection extension

## Call:

INJECTCB_METHOD=MyClass::bar /home/pierre/project/php/php8debug/bin/php  -dextension_dir=/home/pierre/project/php/php_injectcallback/modules -dextension=injectcb asts.php

Env can be defined in the webserver environment as well. A config reload should do it (or fpm).