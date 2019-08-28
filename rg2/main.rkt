#lang racket

;(define cmdline-parser
;    (command-line
;    #:usage-help "The racket based g2 compiler"
;
;    #:args ()]

(define argv (current-command-line-arguments))

(define (path->syntax file-name)
  (parameterize ([port-count-lines-enabled #t])
    (call-with-input-file* file-name
      (λ (prt)
        (for/list ([def (in-port (curry read-syntax file-name)
                                 prt)])
          def)))))

(define ast (path->syntax (vector-ref argv 0)))

(define (assert b msg)
  (when (not b) (error msg)))

(define (assert-clean-toplevel ast)
  (for ([def (in-list ast)])
    (define dat (syntax->datum def))
    (assert (match dat
              [(list 'def (? symbol? _) _) #t]
              [(list 'pub 'def (? symbol? _) _) #t]
              [(list 'var (? symbol? _) _) #t]
              [(list 'var (? symbol? _) type _) #t]
              [(list 'pub 'var (? symbol? _) _) #t]
              [(list 'pub 'var (? symbol? _) type _) #t]
              [(list 'readonly (? symbol? _) _) #t]
              [(list 'readonly (? symbol? _) type _) #t]
              [(list 'pub 'readonly (? symbol? _) _) #t]
              [(list 'pub 'readonly (? symbol? _) type _) #t]
              [_ #f])
            (format "unexpected item at module top-level ~s" def))))

(assert-clean-toplevel ast)


