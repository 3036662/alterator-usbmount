(define-module (ui simple ajax)
    :use-module (alterator ajax)
    :use-module (alterator woo)
    :export (init))

 (define (ls_usbs)
    (form-update-enum "list_prsnt_devices" (woo-list "/simple/list_curr_usbs" ))
 )   

; unblock the selected device 
 (define (allow_device)
    (let ((  status  (woo-read-first "/simple/usb_allow" 'usb_id  (form-value "list_prsnt_devices")) )) 
        (if   
            (equal? "OK" (woo-get-option status 'status))
            (ls_usbs)
            (woo-throw  "Error while trying to unblock selected device")
        )
    ); let
 )

 ;block selected device
 (define (block_device)
     (let ((  status  (woo-read-first "/simple/usb_block" 'usb_id  (form-value "list_prsnt_devices")) )) 
        (if   
            (equal? "OK" (woo-get-option status 'status))
            (ls_usbs)
            (woo-throw  "Error while trying to block selected device")
        )
    ); let
 )

(define (init)
  (ls_usbs) ; update list on init
  (form-bind "btn_prsnt_scan" "click" ls_usbs)
  (form-bind "btn_prsnt_dev_add" "click" allow_device)
  (form-bind "btn_prsnt_dev_block" "click" block_device)
)
