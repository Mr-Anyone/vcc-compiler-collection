" Keywords
syntax keyword vccKeyword function gives while then end if ret int struct array ptr string void external char bool long float short

" Operators
syntax match vccOperator "+\|-\|\*\|/\|eq\|ne\|ge\|gt\|le"

" Brackets and punctuation
syntax match vccDelimiter "[{}()\[\],;]"


" Integer literals
syntax match vccNumber "\<\d\+\>"

" Match comments starting with '#' until the end of the line
syntax match vccComment "#.*$"

" Match string literals (double quotes, allow escapes)
syntax match vccString "\"[^\"]*\""

" Highlight groups
highlight link vccKeyword       Keyword
highlight link vccOperator      Operator
highlight link vccDelimiter     Delimiter
highlight link vccNumber        Number
highlight link vccComment       Comment
highlight link vccString        String
