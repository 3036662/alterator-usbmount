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

// catch a preset selection event
$('input[type=radio][name=presets]').change(function(){
  if($(this).is(':checked')){
    $("#presets_input_hidden").val($(this).val());
  }
}); 
// set preset selectection
$('#presets_input_hidden').bind('update-value change',function(){
    $('input[type=radio][name=presets][value='+$("#presets_input_hidden").val() +']').attr('checked', true);
  });
$('#checkbox_use_control_hidden').trigger('change');

// catch a type-of-list selection event
$('input[type=radio][name=list_type]').change(function(){
  if($(this).is(':checked')){
    $("#hidden_list_type").val($(this).val());
  }
}); 
// preselect type-of-list
$('#hidden_list_type').bind('update-value change',function(){
  $('input[type=radio][name=list_type][value='+$("#hidden_list_type").val() +']').attr('checked', true);
});
$('#hidden_list_type').trigger('change');



}); // .ready