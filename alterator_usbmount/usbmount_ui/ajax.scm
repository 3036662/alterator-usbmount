(define-module (ui usbmount ajax)
    :use-module (alterator ajax)
    :use-module (alterator woo)
    :export (init))

(define (ls-devices)
  (form-update-enum "list_prsnt_devices" (woo-list "/usbmount/list_block" ))
)

(define (init)
 (ls-devices)
)