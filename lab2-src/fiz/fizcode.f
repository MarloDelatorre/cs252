(define (add x y) 
  (ifz y x 
    (add 
      (inc x) (dec y)
    )
  )
)

(define (mul x y)
  (ifz y 0
    (add 
      x
      (mul x (dec y))
    )
  )
)

(define (sub x y) 
  (ifz y 
    x
    (ifz x 
      (halt)
      (sub (dec x) (dec y))
    )
  )
)

(define (div x y)
  (ifz y ; if y = 0, then dividing by 0
    (halt) ; halt because div by 0
    (ifz (lt x y)
      0 ; base case
      (inc (div (sub x y) y)) ; recursive case -- subtract y and increment the base 0
    )
  ) 
)
