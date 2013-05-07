

(define (problem BW-rand-7)
(:domain blocksworld)
(:objects b1 b2 b3 b4 b5 b6 b7 )
(:init
(arm-empty)
(on b1 b2)
(on-table b2)
(on b3 b4)
(on-table b4)
(on b5 b3)
(on b6 b1)
(on b7 b6)
(clear b5)
(clear b7)
)
(:goal
(and
(on b2 b3)
(on b3 b6)
(on b4 b5)
(on b5 b7))
)
)


