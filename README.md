# Example Code 
```
function testing gives int 
[
    int a,
    int b,
    int c,
]{
    a = 10;
    b = 10;
    c = 10;
}
```

# Grammar Specification

function_decl :== 'function', <identifier>, 'gives', <type_qualification>, 
                    <function_args_list>, '{', <expression>+, ''}'

function_args_list :== '[', args_declaration+, ']'

args_declaration :== <type_qualification> + identifier + ','

identifier :== [a-zA-Z]+

integer_literal :== [0-9]+

expression == assignment_expression  

assignment_expression :==   <identifier>, '=', <integer_literal>, ';'

type_qualification :== 'int'

bin_op :== '+'
