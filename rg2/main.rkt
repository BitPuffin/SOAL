#lang racket/base

;(define cmdline-parser
;    (command-line
;    #:usage-help "The racket based g2 compiler"
;
;    #:args ()]

(require racket/list)

(define argv (current-command-line-arguments))

(define (path->syntax file-name)
  (parameterize ([port-count-lines-enabled #t])
    (with-input-from-file file-name
      (λ ()
        (define prt (open-input-file file-name))
        (for/list ([l (in-port (λ (p)
                                 (read-syntax file-name p))
                               prt)])
          l)))))

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

