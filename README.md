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
- [X] The C FFI problem with SDL
- [ ] Add the following operations and, and or.
- [ ] The Heap allocation problem with ASTBase and Type
- [ ] CallExpr error with no matching function
- [ ] undefined variable better message
- [ ] Posfix Expression validity with existence of member  


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

# Example Code 

What a snake game with SDL interface look like: 

```
# ==================================================
# Start of libvcc C interface
#
external function print_line
gives void [ptr char some_string,]

external function print_string
gives void [ptr char some_string,]

external function print_integer 
gives void [int a, ]

# the heap abstraction
external function vcc_malloc 
gives ptr void [long size, ]

external function vcc_free 
gives void [ptr void at, ]

external function get_nullptr
gives ptr void []

# rand function
external function vcc_rand 
gives int [int low, int high, ] 

external function init_rand 
gives void [] 

# ==================================================
# SDL Functions
#
external function SDL_GetTicks
gives long []

external function SDL_Init 
gives bool [int a, ]

external function SDL_PollEvent
gives bool [ptr void a, ]

external function SDL_SetRenderDrawColor
gives bool [ptr void a, char b, char c, char d, char e, ]

external function SDL_RenderClear
gives bool [ptr void a, ]

external function SDL_RenderPresent
gives bool [ptr void a, ] 

external function SDL_CreateWindow
gives ptr void [ptr char title, int width, int height, long flag, ]

#declare ptr @SDL_CreateRenderer(ptr noundef, ptr noundef) #1
external function SDL_CreateRenderer
gives ptr void [ptr void window, ptr void driver,]

struct SDL_KeyboardEvent{
    int type,
    int reserved, 
    long timestamp,
    int padd1,
    int padd2,
    int scancode,
    int key,
    short pad5,
    short pad6,
    bool pad7,
    bool pad8,
}

struct  SDL_FRect{
    float x, 
    float y, 
    float w, 
    float h,
}

external function SDL_RenderFillRect
gives bool [ptr void renderer, ptr struct SDL_FRect b, ]

external function sdl_alloc_event 
gives ptr void []

external function sdl_free_event 
gives void [ptr void event,]

external function sdl_get_keyboard_event 
gives ptr struct SDL_KeyboardEvent [ptr void event,]

external function sdl_is_event 
gives bool [ptr void event, int kind, ]

# ==================================================
# Snake Logic
#
struct Board{
    ptr long board,  # 0 means empty, 1 means snake body, 2 means apple

    # number of rows
    int row, 

    # number of columns
    int col,
}

struct DynamicArray {
    ptr long start,
    int size, 
}

struct World{
    # to SDL code
    ptr void window, 
    ptr void renderer, 
    int windows_width, 
    int windows_height, 

    # the board we are doing 
    struct Board board,

    # is the game still running ?
    bool is_running, 
    bool has_won,

    # array index of the head
    struct DynamicArray snake_row, 
    struct DynamicArray snake_col, 

    # which direction we are moving 0 (left), 1 (right), 2 (up), 3 (down)
    int mov_direction,  
    long last_update_time, # this last time where mov_direction is updated
}

function init_dynamic_array
gives void [
    ptr struct DynamicArray arr, 
]{
    deref<arr>.size = 0;
    ret;
}

function push_array
gives void [ 
    ptr struct DynamicArray arr, 
    long element,
]{
    int new_size = deref<arr>.size + 1;
    ptr long new_array = cast<ptr long>(vcc_malloc(cast<long>(new_size  *8), ) );

    int i = 0;
    while i lt new_size - 1 then 
        new_array[i] = deref<arr>.start[i];
        i = i + 1;
    end

    i = new_size - 1;
    new_array[i] = element;
    
    deref<arr>.size = new_size;
    deref<arr>.start = new_array;
    ret;
}

function get_piece
gives long [
    ptr struct Board board,
    int row, 
    int col,
]{
    int index = row*deref<board>.row + col;
    ret deref<board>.board[index];
}

function put_piece
gives void [
    ptr struct Board board,
    int row, 
    int col,
    long piece,
]{
    int index = row*deref<board>.row + col;
    deref<board>.board[index] = piece;

    ret;
}


# function is_game_over

# notes, this will hang the entire program if 
# there is no way to put an apple
function put_apple 
gives void [
    ptr struct Board board,
]{
    while 1 then 
        int row = vcc_rand(0, deref<board>.row - 1, );
        int col = vcc_rand(0, deref<board>.col - 1, );
        if get_piece(board, row, col, ) eq cast<long>(0) then 
            put_piece(board, row, col, cast<long>(2), ); 
            ret;
        end
    end

    ret;
}

function print_board 
gives void [
    ptr struct Board board, 
]{
    int i = 0;
    while i lt deref<board>.row then 
        int j = 0;
        while j lt deref<board>.col then 
            long piece = get_piece(board, i, j, );
            print_integer(cast<int>(piece), );
            print_string("", );

            j = j + 1;
        end

        print_line("", );
        i = i + 1;
    end
    ret;
}

  
function init_board 
gives void [ptr struct World world,
    int width, 
    int height,]{

    # there is four bytes in in an integer
    long size = cast<long>(width * height * 8); 
    deref<world>.board.board = cast<ptr long>(vcc_malloc(size, ));
    deref<world>.board.row = width;
    deref<world>.board.col = height;

    int i = 0;
    while i lt width*height then 
        deref<world>.board.board[i] = cast<long>(0);
        i = i + 1;
    end

    ret;
}

function process_event
gives void [ptr struct World world, ]{
    int SDL_EVENT_QUIT  = 256;
    int SDL_EVENT_KEY_DOWN  = 768;
    int SDL_SCANCODE_W = 26;
    int SDL_SCANCODE_S = 22;
    int SDL_SCANCODE_A = 4;
    int SDL_SCANCODE_D = 7;

    ptr void event = sdl_alloc_event();
    while SDL_PollEvent(event, ) eq cast<bool>(1)  then
        # checking from SDL_EVENT_QUIT
        if sdl_is_event(event, SDL_EVENT_QUIT, ) eq cast<bool>(1) then 
            deref<world>.is_running = cast<bool>(0);
        end

        if sdl_is_event(event, SDL_EVENT_KEY_DOWN, ) eq cast<bool>(1) then 
            # key mapping code
            # 0 (left), 1 (right), 2 (up), 3 (down)
            ptr struct SDL_KeyboardEvent keyboard_event = 
                sdl_get_keyboard_event(event, );

            if deref<keyboard_event>.scancode eq SDL_SCANCODE_W then 
                if deref<world>.mov_direction ne 3 then 
                    deref<world>.mov_direction = 2; # 2 id up 
                end
            end

            if deref<keyboard_event>.scancode eq SDL_SCANCODE_S then 
                if deref<world>.mov_direction ne 2 then 
                    deref<world>.mov_direction = 3; 
                end
            end

            if deref<keyboard_event>.scancode eq SDL_SCANCODE_A then 
                if deref<world>.mov_direction ne 1 then 
                    deref<world>.mov_direction = 0; 
                end
            end

            if deref<keyboard_event>.scancode eq SDL_SCANCODE_D then 
                if deref<world>.mov_direction ne 0 then 
                    deref<world>.mov_direction = 1; 
                end
            end
        end

    end
    sdl_free_event(event, );

    ret;
}

function have_won
gives bool [ptr struct World world,]{
    int row = 0;
    while row lt deref<world>.board.row then 
        int col = 0;
        while col lt deref<world>.board.col then 
            if get_piece(ref<deref<world>.board>, row, col, )
                eq cast<long>(0) then 
                ret cast<bool>(0);
            end 
            col = col + 1;
        end
        row = row + 1; 
    end

    ret cast<bool>(1);
}

function update_world
gives void [
    ptr struct World world,
]{
    long current_time = SDL_GetTicks();
    long update_threshold = cast<long>(100); # 500ms 
    bool should_put_apple = cast<bool>(0);

    # updating the head every update_threshold sec
    if current_time - deref<world>.last_update_time gt update_threshold then 
        # set the board to black 
        int i = 0;
        while i lt deref<world>.snake_col.size then 
            int row = cast<int>(deref<world>.snake_row.start[i]);
            int col = cast<int>(deref<world>.snake_col.start[i]);

            put_piece(ref<deref<world>.board>, row, col, cast<long>(0),  ); # empty it out
            i = i + 1;
        end
        # saving some variable before it happens 
        int temp_row = cast<int>(deref<world>.snake_row.start[0]);
        int temp_col = cast<int>(deref<world>.snake_col.start[0]);

        int last_row =  cast<int>(deref<world>.snake_row.start[deref<world>.snake_row.size - 1]);
        int last_col =  cast<int>(deref<world>.snake_col.start[deref<world>.snake_col.size - 1]);


        # ========================
        # update the head
        int head_row =  cast<int>(deref<world>.snake_row.start[0]);
        int head_col =  cast<int>(deref<world>.snake_col.start[0]);
        if deref<world>.mov_direction eq 0 then 
            # move left
            deref<world>.snake_col.start[0] = cast<long>(head_col - 1);
            head_col = head_col - 1;
        end

        if deref<world>.mov_direction eq 1 then 
            # move right
            deref<world>.snake_col.start[0] = cast<long>(head_col + 1);
            head_col = head_col + 1;
        end

        if deref<world>.mov_direction eq 2 then 
            # move up
            deref<world>.snake_row.start[0] = cast<long>(head_row - 1);
            head_row = head_row - 1;
        end
        
        if deref<world>.mov_direction eq 3 then 
            # move down
            deref<world>.snake_row.start[0] = cast<long>(head_row + 1);
            head_row = head_row + 1;
        end 

        i = 1;
        while i lt deref<world>.snake_row.size then 
            int temp_temp_row = cast<int>(deref<world>.snake_row.start[i]);
            int temp_temp_col = cast<int>(deref<world>.snake_col.start[i]);

            deref<world>.snake_row.start[i] = cast<long>(temp_row);
            deref<world>.snake_col.start[i] = cast<long>(temp_col);

            temp_row = temp_temp_row;
            temp_col = temp_temp_col;
            i = i + 1;
        end


        # ==========================
        # check for collision
        if get_piece(ref<deref<world>.board>, head_row, head_col, )
            eq cast<long>(2) then 
            # print_line("we have hit an apple", );
            push_array(ref<deref<world>.snake_row>, cast<long>(last_row), );
            push_array(ref<deref<world>.snake_col>, cast<long>(last_col), );

            put_piece(ref<deref<world>.board>, head_row, head_col, cast<long>(0), );
            should_put_apple = cast<bool>(1);
        end

        i = 1;
        while i lt deref<world>.snake_row.size then 
            if deref<world>.snake_row.start[i] eq head_row then 
                if deref<world>.snake_col.start[i] eq head_col then 
                    print_line("you have lost", );
                    deref<world>.is_running = cast<bool>(0);
                end
            end
            i = i + 1;
        end

        # ========================
        # updating the actual world
        i = 0; # to be reused later
        while i lt deref<world>.snake_col.size then 
            int row = cast<int>(deref<world>.snake_row.start[i]);
            int col = cast<int>(deref<world>.snake_col.start[i]);

            put_piece(ref<deref<world>.board>, row, col, cast<long>(1),  ); # empty it out
            i = i + 1;
        end
        i = 0; # to be reused later

        # we might have to put an apple if there was an collision
        if should_put_apple eq cast<bool>(1) then 
            put_apple(ref<deref<world>.board>, );
        end

        # check to see if we have won 
        if have_won(world, ) then 
            print_line("you have won", );
            deref<world>.is_running = cast<bool>(0);
        end

        # putting the snake into the board
        deref<world>.last_update_time = current_time;
    end


    ret;
}


# creates a world, and initialized SDL
# renderer and window
function init_world 
gives void [
    ptr struct World world,
] {
    int SDL_INIT_VIDEO = 32;

    deref<world>.is_running = cast<bool>(1);
    init_board(world, 10, 10, ); 

    bool status = SDL_Init(SDL_INIT_VIDEO,);
    if status eq 0 then 
        print_line("SDL_Init failed ", );
        ret 0;
    end

    # FIXME: check for error here
    deref<world>.windows_width = 700;
    deref<world>.windows_height = 700;
    deref<world>.window = SDL_CreateWindow("some window", 
        deref<world>.windows_width, deref<world>.windows_height, cast<long>(0), ); 
    deref<world>.renderer = SDL_CreateRenderer(deref<world>.window, get_nullptr(), ) ; 

    # setting the head
    init_dynamic_array(ref<deref<world>.snake_row>, );
    init_dynamic_array(ref<deref<world>.snake_col>, );

    int start_row = deref<world>.board.row / 2;
    int start_col = deref<world>.board.col / 2;
    push_array(ref<deref<world>.snake_row>,  cast<long>(start_row), );
    push_array(ref<deref<world>.snake_col>,  cast<long>(start_col), );

    put_piece(ref<deref<world>.board>, start_row, start_col, cast<long>(1), );
    put_apple(ref<deref<world>.board>, );

    deref<world>.mov_direction = 1; # left
    deref<world>.last_update_time = cast<long>(0); # left
    deref<world>.has_won = cast<bool>(0); # has not won
    ret;
}

function draw_world 
gives void [
    ptr struct World world,
] {  
    # rendering the main things
    # setting black background
    SDL_SetRenderDrawColor(deref<world>.renderer, cast<char>(0), cast<char>(0), 
        cast<char>(0), cast<char>(255), );
    SDL_RenderClear(deref<world>.renderer,);


    int box_width = deref<world>.windows_width / deref<world>.board.col;
    int box_height = deref<world>.windows_height / deref<world>.board.row;

    # RENDERING the board
    #
    # this really is just a for loop but the syntax is quite awkward
    int render_row = 0;
    while render_row lt deref<world>.board.row then 
        int render_col = 0;
        while render_col lt deref<world>.board.col then 
            long piece = get_piece(ref<deref<world>.board>, render_row, render_col, );
            
            # we have a snake body
            if piece eq cast<long>(1) then 
                # we need to draw the body and not head
                struct SDL_FRect rect;
                rect.x = cast<float>(render_col * box_width);
                rect.y = cast<float>(render_row * box_height);
                rect.w = cast<float>(box_width);
                rect.h = cast<float>(box_height);

                SDL_SetRenderDrawColor(deref<world>.renderer, cast<char>(255), cast<char>(0),
                        cast<char>(0), cast<char>(255), );
                SDL_RenderFillRect(deref<world>.renderer, ref<rect>, );
            end
            
            # we have a apple
            if piece eq cast<long>(2) then 
                # we need to draw the body and not head
                struct SDL_FRect rect;
                rect.x = cast<float>(render_col * box_width);
                rect.y = cast<float>(render_row * box_height);
                rect.w = cast<float>(box_width);
                rect.h = cast<float>(box_height);

                SDL_SetRenderDrawColor(deref<world>.renderer, cast<char>(255), cast<char>(0),
                        cast<char>(255), cast<char>(255), );
                SDL_RenderFillRect(deref<world>.renderer, ref<rect>, );
            end

            # rendering the column
            render_col = render_col + 1;
        end

        render_row = render_row + 1;
    end

    SDL_RenderPresent(deref<world>.renderer,);
    ret;
}


function main
gives int[]{
    # SDL Constants
    int SDL_EVENT_QUIT  = 256;
    int SDL_EVENT_KEY_DOWN  = 768;
    int SDL_SCANCODE_W = 26;

    struct World world;
    init_rand();
    init_world(ref<world>, );

    print_line("I have initialized SDL", );

    while (world.is_running eq cast<bool>(1)) then  
        # processing the events like key presses, etc
        process_event(ref<world>, );

        # drawing the snake
        draw_world(ref<world>, );

        # update position for every some threshold amount of time
        update_world(ref<world>, );
    end

    ret 0;
}
```
