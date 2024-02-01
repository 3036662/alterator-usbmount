(define-module (ui simple ajax)
    :use-module (alterator ajax)
    :use-module (alterator woo)
    :export (init))

(define (ls_usbs)
    (form-update-enum "list_prsnt_devices" (woo-list "/simple/list_curr_usbs" ))
)   

; get udev rules filenames
(define (udev_rules_check)
    (form-update-enum "suspicious_udev_files" (woo-list "/simple/check_config_udev"))
)

; remove first el from sublists ( (a b c) (a b c)) => ((b c)(b c))
; purpose is to construct associative list
(define (removeFirstElement lst)
  (map cdr lst))
; get value by key from associative  
(define (get-value key assoc-list)
  (cadr (assoc key assoc-list)))

; get status
; usbguard: "OK" or "BAD"
; udev:     "OK" or BAD 
(define (config_status_check)
   (let ((status  (removeFirstElement (woo-read "/simple/config_status")) ))
           ; (woo-throw (get-value 'usbguard status))
           ; udev status
           (if (string=? "OK" (get-value 'udev status))
               (format (current-error-port) "debug-message~%")
               (udev_rules_check)  
           )       
   )  
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
  (config_status_check) 
  (form-bind "btn_prsnt_scan" "click" ls_usbs)
  (form-bind "btn_prsnt_dev_add" "click" allow_device)
  (form-bind "btn_prsnt_dev_block" "click" block_device)
)
