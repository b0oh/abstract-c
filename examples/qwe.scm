(let ((pairmake
       (lambda (first second pair ) (pair first second ))))
  (let ((first
         (lambda (pair )
           (pair (lambda (first second ) first ) ) ) ) )
    (let ((second
           (lambda (pair )
             (pair (lambda (first second ) second ) ) ) ) )
      (let ((+
             (lambda (pair succ zero )
               ((first pair ) succ ((second pair ) succ zero ) ) ) ) )
        (let ((1
               (lambda (succ zero )
                 (succ zero ) ) ) )
          (let ((2
                 (lambda (succ zero )
                   (succ (succ zero ) ) ) ) )
            (let ((some-pair (pairmake 1 2 ) ) )
              (+ some-pair ) ) ) ) ) ) ) )
