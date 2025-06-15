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

```
function_decl :== 'function', <identifier>, 'gives', <type_qualification>, 
                    <function_args_list>, '{', <statements>+, ''}'

function_args_list :== '[', args_declaration+, ']'

args_declaration :== <type_qualification> + identifier + ','

identifier :== [a-zA-Z]+

integer_literal :== [0-9]+

statements :== <assignment_statement> | <return_statement>

assignment_statement :== <identifier>, '=', <integer_literal>, ';'

return_statement :== 'ret', <identifier>, ';'

type_qualification :== 'int'

expression :==  <binary_expression>

binary_expression :== <trivial_expression> | <trivial_expression>, <bin_op>, <binary_expression>

trivial_expression :== <identifier> | <call_expression> |
                        '(', <expression> ')'

call_expressions :== <identifier>, '()'

bin_op :== '+', '-', '*', '/'
```
