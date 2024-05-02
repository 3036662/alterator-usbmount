(define-module (ui usbmount ajax)
    :use-module (alterator ajax)
    :use-module (alterator woo)
    :export (init))

(define (object->string obj)
  (call-with-output-string
    (lambda (port)
      (write obj port))))    
(define (get-value key assoc-list)
  (cadr (assoc key assoc-list)))

; remove first element from sublists ( (a b c) (a b c)) => ((b c)(b c))
; purpose is to construct associative list
(define (removeFirstElement lst)
  (map cdr lst))

(define (ls-devices)
  (form-update-enum "list_prsnt_devices" (woo-list "/usbmount/list_block" ))
)

(define (ls-rules)
 (js "UpdateRulesList" (car(removeFirstElement (woo-read "/usbmount/list-devices"))))
)

(define (save-rules)
;(woo-error (object->string   (car(removeFirstElement(woo-read "/usbmount/save_rules" 'data (form-value "new_rules_data")))) ))
     (if (string=? "OK" (car(car(removeFirstElement(woo-read "/usbmount/save_rules" 'data (form-value "new_rules_data"))))))  
      (ls-rules)
      (woo-error (_ "Save rules operation FAILED"))
     ); //if
)

(define (init)
 (js "InitUi")
 (js "SetUsersAndGroups" (car(removeFirstElement(woo-read "/usbmount/get_users_groups"))))
 (ls-rules)
 (ls-devices)
 (form-bind "btn_prsnt_scan" "click" ls-devices)
 (form-bind "btn_save" "rules_data_ready" save-rules)
)