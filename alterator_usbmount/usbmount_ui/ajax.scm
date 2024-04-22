(define-module (ui usbmount ajax)
    :use-module (alterator ajax)
    :use-module (alterator woo)
    :export (init))

(define (object->string obj)
  (call-with-output-string
    (lambda (port)
      (write obj port))))    

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

(define (init)
 (js "InitUi")
 (js "SetUsersAndGroups" (car(removeFirstElement(woo-read "/usbmount/get_users_groups"))))
 (ls-rules)
 (ls-devices)
 (form-bind "btn_prsnt_scan" "click" ls-devices)

)