" Keywords
syntax keyword mydslKeyword function gives while then end if ret int struct array ptr

" Operators
syntax match mydslOperator "+\|-\|\*\|/\|eq\|ne\|ge\|gt\|le"

" Brackets and punctuation
syntax match mydslDelimiter "[{}()\[\],;]"


" Integer literals
syntax match mydslNumber "\<\d\+\>"

" Highlight groups
highlight link mydslKeyword       Keyword
highlight link mydslOperator      Operator
highlight link mydslDelimiter     Delimiter
highlight link mydslNumber        Number
