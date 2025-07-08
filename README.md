# TODO

- [] Add support for aggregate type
- [] Floating point support
- [] Better error handling and printing
- [] The problem of implicit casting and semantics analysis
- [] The C FFI problem with SDL

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

assignment_statement :== <identifier>, '=', <expression>, ';'

return_statement :== 'ret', <expression> ';'

type_qualification :== 'int'

expression :==  <binary_expression>

binary_expression :== <trivial_expression> | 
                            <trivial_expression>, <bin_op>, <binary_expression>

trivial_expression :== <identifier> | <call_expression> |
                            '(', <expression>, ')' | <integer_literal>

call_expressions :== <identifier>, '(', {<expression> ',' }+,  ')'

bin_op :== '+', '-', '*', '/', 'eq', 'ne', 'ge', 'gt', 'le', 'gt'
```

# Semantics

## Binary comparison

1. When an implicit conversion happens for `i1` with a type `in` , `i1` will be zero extended.

## General Rules 

1. Variables names cannot contain '$', they are reserved for implementation

## Name Lookup

1. Within the same function, two declaration cannot share the same name. 
