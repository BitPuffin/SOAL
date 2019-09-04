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
              [(list (or 'def 'var 'readonly) (? symbol? name) _) #t]
              [(list (or 'def 'var 'readonly) (? symbol? name) type _) #t]
              [(list 'pub (or 'def 'var 'readonly) (? symbol? name) _) #t]
              [(list 'pub (or 'def 'var 'readonly) (? symbol? name) type _) #t]
              [_ #f])
            (format "unexpected item at module top-level ~s" def))))

(assert-clean-toplevel ast)

(define (extract-module-definitions ast)
  (map (λ (def)
         (define els (let* ([v (syntax-e def)]
                            [v (if (eq? 'pub (syntax->datum (first v)))
                                   (rest v)
                                   v)]
                            [v (rest v)]) ; pop off def/var/readonly
                       v))
         (define ident (syntax->datum (first els)))
         (define type-inferred? (= 2 (length els)))
         (define body
           (if type-inferred?
               (second els)
               (first els)))
         (define type (if type-inferred?
                          '__compiler-inferred
                          (syntax->datum (first els))))
         (list ident body type))
       ast))

(define my-definitions (extract-module-definitions ast))
(define my-module-identifiers (map first my-definitions))

(let ([dup (check-duplicates my-module-identifiers #:default #f)])
  (assert (not dup) (format "the identifier ~a is defined twice" dup)))


