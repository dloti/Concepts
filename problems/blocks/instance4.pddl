(define (problem BLOCKS-4-3)
(:domain BLOCKS)
(:objects D B C A )
(:init (clear A) (clear C) (on-table A) (on-table B) (on-table D)
 (on C B) (arm-empty) )
(:goal (AND (on B A) (on C B)))
)
