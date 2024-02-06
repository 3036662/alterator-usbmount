$(document).ready(function() {

// workaround to check and uncheck checkbox as usual input
// state of check  can be changed from lisp 
// with form-update-value for hidden input 
$('#checkbox_use_control').change(function(){
    if ($("#checkbox_use_control").attr('checked')){
      $("#checkbox_use_control_hidden").val("#t");
    }
    else{
      $("#checkbox_use_control_hidden").val("#f");      
    }
  });
  $('#checkbox_use_control_hidden').bind('update-value change',function(){
     if ($("#checkbox_use_control_hidden").val()==="#t"){
       $("#checkbox_use_control").attr('checked',true);
     }
     else{
       $("#checkbox_use_control").attr('checked',false);      
     }
  });
  $('#checkbox_use_control_hidden').trigger('change');
});