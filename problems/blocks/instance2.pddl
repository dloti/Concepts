(define (problem BLOCKS-4-2)
(:domain BLOCKS)
(:objects B C A )
(:init (clear A) (clear B) (clear C) (on-table A) (on-table B) (on-table C)
(arm-empty) )
(:goal (AND (on A B) (on B C) ))
)