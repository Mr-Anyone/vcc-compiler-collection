# TODO

- [X] Name lookup problem
- [X] Trie like Symbol Table
- [X] Implicit casting for integer
- [X] Better error handling and printing
- [X] Add support for aggregate type
- [X] Parse command line
- [X] Array Type
- [X] Pointer Type
- [ ] The C FFI problem with SDL
- [ ] Semantics analysis
- [ ] String Type  
- [ ] Floating point support
- [ ] The heap allocation problem with ASTBase and Type

# Example Code 
```
function testing gives int 
[
    int a,
    int b,
    int c,
    array int c,
]{
    a = 10;
    b = 10;
    c = 10;
}
```

# Grammar Specification

```
top_level :== <function_decl> | <struct_definition>
function_decl :== 'function', <identifier>, 'gives', <type_qualification>, 
                        <function_args_list>, '{', <statements>+, ''}'

function_args_list :== '[', <args_declaration>+, ']'

args_declaration :== <type_qualification>, identifier, ','

identifier :== [a-zA-Z]+

integer_literal :== [0-9]+

statements :== <assignment_statement> | <return_statement> | <if_statement> | <while_statement> 
        | <declaration_statement>

while_statement :== 'while', <expression>, 'then', <statements>+,'end'

declaration_statement :== <type_qualification>, <identifier>, {'=', <expression>} , ';'

if_statement :== 'if', <expression>, 'then', <statements>+, 'end'

assignment_statement :== <identifier>, '=', <expression>, ';'  

return_statement :== 'ret', <expression> ';'

type_qualification :== 'int' | 'struct', <identifier> |
                    'array', '(', <integer_literal>, ')', <type_qualification> |
                    'ptr', <type_qualification>

struct_definition :== 'struct', <identifier> ,'{'
                        , {<type_qualification> <identifier>}+, '}'

expression :==  <binary_expression>

binary_expression :== <trivial_expression> | 
                            <trivial_expression>, <bin_op>, <binary_expression>

trivial_expression :== <identifier> | <call_expression> |
                            '(', <expression>, ')' | <integer_literal> | <member_access_expression>

member_access_expression :== <identifier> | <identifier>, '.', <member_access_expression>

call_expressions :== <identifier>, '(', {<expression> ',' }+,  ')'

bin_op :== '+', '-', '*', '/', 'eq', 'ne', 'ge', 'gt', 'le', 'gt'
```

# Semantics

## Binary operator

1. Implicit conversion always happen for type that has less bits. For example, when adding `i16` with `i32`, `i16` will be sign extended to `i32` to perform the addition. Note boolean are exempted from this.
2. Inside a if statement, the condition is false if and only if the value is 0. In other words, it is true if and only if the value is non zero.


## Name Lookup

1. If, while, and function defines a scope that begins with '{'  and ends with '}'. Excluding code in between '{', and '}'.
2. Within the same scope, two declaration cannot share the same name. 
3. Name lookup will start at the current scope and climb upwards.


# Design Decision

1. ASTBase must be immutable. Once created, it must not be changed. It will cause bugs otherwise.

## Known Problems 

1. Signed and unsigned integer comparison.

