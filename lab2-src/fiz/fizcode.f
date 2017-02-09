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

(define (lt x y)
  (ifz y 1
    (ifz x 0
      (lt (dec x) (dec y))
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

(define (rem x y)
  (ifz y (halt)
    (ifz (lt x y) x
      (rem (sub x y) y)
    )
  )
)

(define (gcd x y)
  (ifz (lt x y) (gcd y x)
    (ifz y x (gcd y (rem x y)))
  )
)

(define (lcm x y)
  (div (mul x y) (gcd x y))
)

(define (srootaux z y)
  (ifz (lt z (mul y y)) (dec y) 
    (srootaux z (inc y))
  )
)

(define (sroot z)
  (srootaux z 0)
)

(define (eqone z)
  (ifz z (halt)
    (ifz (dec z) 1 0)
  )
)

(define (minvaux x z y)
  (ifz (eqone (gcd x z)) (halt)
    (ifz (eqone (rem (mul x y) z)) 
      (minvaux x z (inc y))
      y
    )
  )
)

(define (minv x z)
  (minvaux x z 1)
)
