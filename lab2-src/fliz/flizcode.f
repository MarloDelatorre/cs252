(define (append x y)
  (ifn x y
    (list (head x) (append (tail x) y))
  )
)

(define (reverse x)
  (ifn x x 
    (append (reverse (tail x))
      (list (head x) [])
    )
  )
)

(define (recreverse x)
  (ifn x x
    (append (recreverse (tail x))
      (ifa (head x) (list (head x) []) 
        (list (recreverse (head x)) [])
      )
    )
  )
)

(define (intlist x)
  (ifn x 1 
    (ifa (head x) (intlist (tail x)) 0)
  )
)
