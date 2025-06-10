# Example Code 
```
function testing gives int 
[
    int a,
    int b,
    int c,
]{
    a + b + c;
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

assignment_expression :==  <integer_literal>, <bin_op>, <integer_literal>

type_qualification :== 'int'

bin_op :== '+'
