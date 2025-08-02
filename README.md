# TODO
- [X] Name lookup problem
- [X] Trie like Symbol Table
- [X] Implicit casting for integer
- [X] Better error handling and printing
- [X] Add support for aggregate type
- [X] Parse command line
- [X] Array Type
- [X] Pointer Type
- [X] Floating point support
- [X] Add comments
- [X] String Type  
- [X] Bool type with problem BinaryExpression::getType() 
- [X] Diagnostics Driver
- [X] Add source location 
- [X] Add the following type short, long, and void*
- [X] Adding a cast expression
- [ ] The C FFI problem with SDL
- [ ] Add the following operations and, and or.
- [ ] The Heap allocation problem with ASTBase and Type
- [ ] CallExpr error with no matching function
- [ ] undefined variable better message
- [ ] Posfix Expression validity with existence of member  

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

    ret 1;
}
```

# Grammar Specification

```
top_level :== <function_decl> | <struct_definition> | <external_decl>

external_decl :== 'extern', 'function', <identifier>, 
    'gives', <type_qualification>, '[', <functin_args_list>, ']';

function_decl :== 'function', <identifier>, 'gives', <type_qualification>, 
                        <function_args_list>, '{', <statements>+, ''}'

function_args_list :== '[', <args_declaration>+, ']'

args_declaration :== <type_qualification>, identifier, ','

identifier :== [a-zA-Z]+

integer_literal :== [0-9]+

statements :== <assignment_statement> | <return_statement> | <if_statement> | <while_statement> 
        | <declaration_statement> | <call_statement>

call_statement :== <call_expression>, ';'

while_statement :== 'while', <expression>, 'then', <statements>+,'end'

declaration_statement :== <type_qualification>, <identifier>, {'=', <expression>} , ';'

if_statement :== 'if', <expression>, 'then', <statements>+, 'end'

assignment_statement :== <trivial_expression>, '=', <expression>, ';'
    | <posfix_expression> ,'=' <expression>, ';'

return_statement :== 'ret', {<expression>} ';'

type_qualification :== 'int' | 'struct', <identifier> |
                    'array', '(', <integer_literal>, ')', <type_qualification> |
                    'ptr', <type_qualification> | 'float' | 'void' | 
                    'char' | 'bool' | 'long' | 'short'

struct_definition :== 'struct', <identifier> ,'{'
                        , {<type_qualification> <identifier>}+, '}'

expression :==  <binary_expression>

binary_expression :== <trivial_expression> | 
                            <trivial_expression>, <bin_op>, <binary_expression>

deref_expression :== 'deref', '<', <trivial_expression>, '>'

ref_expression :== 'ref', '<', <trivial_expression>, '>'

string_literal :== '"', [.]+, '"'

trivial_expression :== <identifier> | <call_expression> |
                            '(', <expression>, ')' | <integer_literal> | 
                            <posfix_expression> | <deref_expression> | <ref_expression> 
                            | <string_literal>

cast_expression :== 'cast', '<', <type_qualification> '>', '(', <expression>,')'
    
posfix_expression :== <identifier> | <deref_expression>
    <posfix_expression>, '.', <identifier> | 
    <posfix_expression>, '[', <expression>, ']'

call_expressions :== <identifier>, '(', {<expression> ',' }+,  ')'

bin_op :== '+', '-', '*', '/', 'eq', 'ne', 'ge', 'gt', 'le', 'gt'
```

# Semantics

## Definition

1. Locator value are values that defines both a type, location (memory reference), and a value.

```
int a = 30; # a is a locator value, it is somewhere in the stack and has a type i32 with 30 as its value
```

## General Expression Semantics


1. Implicit conversion always happen for type that has less bits. For example, when adding `i16` with `i32`, `i16` will be sign extended to `i32` to perform the addition. This applies to boolean expression.

```
(a eq b) + c # (a eq b ) will be sign extended into i32, likely resulting in overflow
```

2. Implicit conversion will take place between right hand side and left hand side of a binary expression if the two given type are float and int.  In such case, the implicit conversion will take place in int and be converted into a float.

```
int a = 10; 
float b = 20; 
a + b; # a will be converted into float implicitly
```

3. Integer are always signed.

4. Binary Expression with pointer is ill form.

```
ptr int a; 
ptr int b = a + 10; # this is ill form
```

5. String is a pointer to the first character

```
ptr char some_str = "testing"; # Here some_str points to the 't' character
```

6. Cast expression converts a type A into type B assuming that type A can be converted into type B. Conversion between array to pointer is ill formed.

```
array (10) array_a ;
ptr int a = cast<a>; # this is well form
```

ptr void is allowed to be casted from or to any pointer type.

```
ptr int a; 
ptr float b; 

# the following are all well formed
ptr void c = cast<ptr void>(a);
c = cast<ptr void>(deref<a>);
c = cast<ptr void>(b);
a = cast<ptr float>(c);
b = cast<ptr int>(c);

```
## Statements

1. Inside a if statement, the condition is false if and only if the value is 0. In other words, it is true if and only if the value is non zero.

```
int a = 0;
if a + 1 then # yields true, but if a = 0, yields false
end
```

2. The type of AssignmentStatement of left and right hand side must be the same.

3. It is ill form for RefExpression to be on the left hande side of assignment statement.

```
int a = 30;
ref<a> = 20; # this is ill-form
```

4. The left hand side of assignment statement must a Locator.

5. Void type is only used for function declaration return type

## Name Lookup

1. If, while, and function defines a scope that begins with '{'  and ends with '}'. Excluding code in between '{', and '}'.
2. Within the same scope, two declaration cannot share the same name. 
3. Name lookup will start at the current scope and climb upwards.


# Design Decision

1. ASTBase must be immutable. Once created, it must not be changed. It will cause bugs otherwise.

## Known Problems 

1. Signed and unsigned integer comparison.

2. Wrong ast ordering
```
struct vec2{
    int a,
    int b,
}

function some_test_here
gives struct vec2[
]{
    struct vec2 a;
    a.a = 10;
    a.b = 20;

    ret a;
}
```

Gives: 

```
FunctionDecl name: some_test_here args:
  FunctionArgLists
  DeclarationStatement name: a
  AssignmentStatement
      ConstantExpr 10
      MemberAccessExpression a.a is_ref: 1 child: 0 this: 0x58512afd79c0
  AssignmentStatement
      MemberAccessExpression a.b is_ref: 1 child: 0 this: 0x58512afd7b80
      ConstantExpr 20
  ReturnStatement
      IdentifierExpr identifier: a compute-ref: false
```

Why is ConstantExpr infront of MemberAccessExpr?

```
struct Board{
    ptr int board,  # 0 means empty, 1 means snake body, 2 means apple

    int width, 
    int height,
}

struct World{
    # to SDL code
    ptr void window, 
    ptr void renderer, 
    struct Board board,

    # is the game still running ?
    bool is_running, 
}

ptr struct World world
deref<world>.board.board[i] = 0;
```
generates  incorrect code
