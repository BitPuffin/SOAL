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

(define (verify-module-has-only-definitions ast)
  (for ([def (in-list ast)])
    (define def-datum (syntax->datum def))
    (unless (and (list? def-datum)
                 (or (eq? (first def-datum) 'def)
                     (and (eq? (first def-datum) 'pub)
                          (eq? (second def-datum) 'def))))
      (error (format "unexpected: ~s" def)))))

(verify-module-has-only-definitions ast)

