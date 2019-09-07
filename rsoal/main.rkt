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

;(define ast (path->syntax (vector-ref argv 0)))
(define ast (path->syntax "test.soal"))

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

;; TODO, this can probably be generalized to extract scope definitions
;; or something like that
;; maybe make extract scope constants, since those can be referenced
;; in any order
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
         (list ident def))
       ast))

(define my-definitions (extract-module-definitions ast))

(define builtin-identifiers (list '+ '- '* '/))
(define my-module-identifiers
  (append builtin-identifiers (map first my-definitions)))

(let ([dup (check-duplicates my-module-identifiers #:default #f)])
  (assert (not dup) (format "the identifier ~a is defined twice" dup)))

(assert (member 'main my-module-identifiers) "No main procedure defined in module")

(define main-def (second (assoc 'main my-definitions)))

(define (pub? stx)
  (define ls (syntax->datum stx))
  (eq? (first ls) 'pub))

(assert (pub? main-def) "main has to be public") 

(define def->value (compose last syntax-e))

(define (proc? stx)
  (define ls (if (list? stx)
                 stx
                 (syntax->datum stx)))
  (and (list? ls)
       (eq? 'proc (first ls))))

(define main-value (def->value main-def))

(assert (proc? main-value)
        (format "main must be a procedure\n~a\n^^^ not a valid value for main"
                (syntax->datum main-value)))

(define proc-return-type (compose second
                                  syntax->datum))

(let ([return-type (proc-return-type main-value)])
  (assert (or ;(eq? return-type 'int)
              (eq? return-type 'void))
          (format "expected return type to be void, '~a' is not acceptable"
                  return-type)))

(define proc-args (compose third
                           syntax->datum))

(assert (empty? (proc-args main-value))
        "Expected main to not take any arguments")

(define (proc-body p)
  (drop (syntax->datum p) 3))

(define keywords '(var def readonly proc))

;; TODO: this one doesn't pass when you define a variable in the body
(define (validate-body body)
  (define identifier? (λ (p) (and (not (member p keywords))
                                  (symbol? p))))
  (define syms (filter identifier? (flatten body)))
  (for ([s (in-list syms)])
    (assert (member s my-module-identifiers)
            (format "unknown symbol: ~a" s))))

(validate-body (proc-body main-value))

(define (get-proc-dependencies body)
  (define identifier? (λ (p) (and (not (member p keywords))
                                  (symbol? p))))
  (define (not-builtin? p)
    (not (member p builtin-identifiers)))
  (filter (λ (p) (and (not-builtin? p) (identifier? p)))
          (flatten (fourth (syntax->datum body)))))

(define deps (get-proc-dependencies main-value))

(define (def? lst)
  (and (list? lst) (eq? 'def (car lst))))

(define decl-type third)

(define (emit-dependencies deps)
  (for ([dep (in-list deps)])
    (define def (assoc dep my-definitions))
    (define val-stx (second def))
    (define val (syntax->datum val-stx))
    ;(define val (second def))
    (define typ (decl-type val))
    (define qbe-typ (match typ
                      ('int 'w)))
    (cond
      [(and (def? val) (not (proc? val))) 
       (displayln (format "data $~a = { ~a ~a }"
                          dep
                          qbe-typ
                          (last val)))])))

(define main-output (open-output-file "main.qbe" #:exists 'replace))

(define stdout (current-output-port))
(current-output-port main-output)

(emit-dependencies deps)

(define (emit-proc proc)
  (displayln proc))

(define form-cmd first)

(define (emit-add form)
  (assert (= 3 (length form)) "invalid add form")
  (displayln (format "%l =w loadw $~a" (second form)))
  (displayln (format "%r =w loadw $~a" (third form)))
  (displayln "%n =w add %l, %r"))

(define primitives (list (list '+ emit-add)))

(define (emit-proc-body proc)
  (define bdy (proc-body proc))
  (for ([expr (in-list bdy)])
    (define cmd (form-cmd expr))
    (define emitter (second (assoc cmd primitives))) ; I barely know her!
    (emitter expr)))

(define (emit-main main)
  (displayln "export function w $main() {")
  (displayln "@start")
  (emit-proc-body main)
  (displayln "ret")
  (displayln "}"))

(emit-main main-value)

(close-output-port main-output)
(current-output-port stdout)

(system "qbe -o asm.s main.qbe")
(system "cc -o main asm.s")

