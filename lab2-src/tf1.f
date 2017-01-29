; This test file tests the ability to parse a list and displays it
; This test file shows what is entered, and after ; what is expecetd
[1 2 3]		; [ 1 2 3 ]
[]              ; [ ]
[1 [2 3] [4 5]] ; [ 1 [ 2 3 ] [ 4 5] ] 
[ [] [1 [11 [111] [222]]]]  ;  [ [ ] [ 1 [ 11 [ 111 ] [ 222 ] ] ] ]
