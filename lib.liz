(define {nil} {})
(define {defun} (lambda {f b} {define (head f) (lambda (tail f) b)}))
(defun {unpack f l} {
  eval (join (list f) l)
})
(defun {pack f & xs} {f xs})
(define {curry} unpack)
(define {uncurry} pack)
(defun {do & l} {
  cond (= l nil)
    {nil}
    {last l}
})
(defun {let b} {
  ((lambda {_} b) ())
})
(defun {not x}   {- 1 x})
(defun {or x y}  {+ x y})
(defun {and x y} {* x y})
(defun {comp f g x} {f (g x)})
(defun {first l} { eval (head l) })
(defun {second l} { eval (head (tail l)) })
(defun {third l} { eval (head (tail (tail l))) })
(defun {len l} {
  cond (= l nil)
    {0}
    {+ 1 (len (tail l))}
})
(defun {nth n l} {
  cond (= n 0)
    {first l}
    {nth (- n 1) (tail l)}
})
(defun {last l} {nth (- (len l) 1) l})
(defun {map f l} {
  cond (= l nil)
    {nil}
    {join (list (f (first l))) (map f (tail l))}
})
(defun {filter f l} {
  cond (= l nil)
    {nil}
    {join (cond (f (first l)) {head l} {nil}) (filter f (tail l))}
})
(defun {foldl f z l} {
  cond (= l nil)
    {z}
    {foldl f (f z (first l)) (tail l)}
})
(defun {select & cs} {
  cond (= cs nil)
    {error "No Selection Found"}
    {cond (first (first cs)) {second (first cs)} {unpack select (tail cs)}}
})
(define {otherwise} #true)
(define {mod} %)
(define {rem} %)
(define {remainder} %)
(define {modulo} %)
(define {add} +)
(define {sub} -)
(define {subtract} -)
(define {neg} -)
(define {negate} -)
(define {div} /)
(define {divide} /)
(define {multiply} *)
(define {mul} *)
(define {if} cond)
(define {fst} first)
(define {snd} second)
(define {trd} third)
(defun {sum l} {foldl + 0 l})
(defun {product l} {foldl * 1 l})
