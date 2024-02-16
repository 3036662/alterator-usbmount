$(document).ready(function () {


  // workaround to check and uncheck checkbox as usual input
  // state of check  can be changed from lisp 
  // with form-update-value for hidden input 
  $('#checkbox_use_control').change(function () {
    if ($("#checkbox_use_control").attr('checked')) {
      $("#checkbox_use_control_hidden").val("#t");
    }
    else {
      $("#checkbox_use_control_hidden").val("#f");
    }
  });
  $('#checkbox_use_control_hidden').bind('update-value change', function () {
    if ($("#checkbox_use_control_hidden").val() === "#t") {
      $("#checkbox_use_control").attr('checked', true);
    }
    else {
      $("#checkbox_use_control").attr('checked', false);
    }
  });
  $('#checkbox_use_control_hidden').trigger('change');

  /*******************************************************/
  // catch a preset selection event
  $('input[type=radio][name=presets]').change(function () {
    if ($(this).is(':checked')) {
      $("#presets_input_hidden").val($(this).val());
    }
  });
  // set preset selection
  $('#presets_input_hidden').bind('update-value change', function () {
    $('input[type=radio][name=presets][value=' + $("#presets_input_hidden").val() + ']').attr('checked', true);
  });
  $('#checkbox_use_control_hidden').trigger('change');

  // catch a type-of-list selection event
  $('input[type=radio][name=list_type]').change(function () {
    if ($(this).is(':checked')) {
      $("#hidden_list_type").val($(this).val());
    }
  });
  // preselect type-of-list
  $('#hidden_list_type').bind('update-value change', function () {
    $('input[type=radio][name=list_type][value=' + $("#hidden_list_type").val() + ']').attr('checked', true);
  });
  $('#hidden_list_type').trigger('change');

  /*******************************************************/
  // delete strings from tables

  localStorage.removeItem("deletedFields");

  $("#delete_rules_from_hash_level").bind('click', "form_list_hash_rules", SaveDeleted);
  $("#delete_rules_from_vid_pid_level").bind('click', "form_list_vidpid_rules", SaveDeleted);
  $("#delete_rules_from_interface_level").bind('click', "form_list_interface_rules", SaveDeleted);
  $("#delete_rules_from_unsorted_level").bind('click', "form_list_unsorted_rules", SaveDeleted);


  /*******************************************************/
  // add strings to tables
  console.log($("#hidden_list_type").val());

  // add new hash-level rule
  var new_hash_row = 
    '<td><span class="alterator-label"><input type="text" class="input_appended" name="hash"><span></td>' +
    '<td>--</td>';
  addRuleBehaviorAdd("#add_to_rules_hash", new_hash_row, "#list_hash_rules");

  // add new vidpid-level rule
  var new_vidpid_row = 
    '<td><span class="alterator-label"><input type="text" class="input_appended" name="vid"><span></td>' +
    '<td>--</td>' +
    '<td><span class="alterator-label"><input type="text" class="input_appended" name="pid"><span></td>' +
    '<td>--</td><td>--</td>';
  addRuleBehaviorAdd("#add_to_rules_vidpid", new_vidpid_row, "#list_vidpid_rules");

  // add new CC::SS::PP rule
  var new_interface_row = 
    '<td><span class="alterator-label"><input type="text" class="input_appended" name="interface"><span></td>' +
    '<td>--</td><td>--</td>';
  addRuleBehaviorAdd("#add_to_rules_interfase", new_interface_row, "#list_interface_rules");

  // add new rule to unsorted-rules
  var new_unsorted_rules_row = 
    '<td><span class="alterator-label"><input type="text" class="input_appended" name="raw_rule"><span></td>';
  addRuleBehaviorAdd("#add_to_rules_unsorted", new_unsorted_rules_row, "#list_unsorted_rules");


  window.last_append_rule_id = 0;
  // read stored values, put them to hidden input, trigger "ready" event for it
  $("#save_rules_button").bind('click', function () {
    var deletedFields = localStorage.getItem('deletedFields');
    var rules_changes={
      deleted_rules:  JSON.parse(deletedFields),
      appended_rules: collectAppendedRules()
    }
    $("#hidden_manual_changes_data").val(JSON.stringify(rules_changes));
   // $(".manual_appeded").remove();
    $("#hidden_manual_changes_data").trigger('ready');
  });


}); // .ready

/*******************************************************/

function addRuleBehaviorAdd(button_id, row_html, table_id) {
  $(button_id).bind('click', function () {
    ++window.last_append_rule_id;
    $(table_id).append(
      '<tr class="manual_appeded" id="rule_'+window.last_append_rule_id+'"><td><input class="select_appended" type="checkbox"></td>' +
      '<td>--</td>' +
      row_html + '<td class="appended_rule_target">' +
      ($("#hidden_list_type").val() === "radio_white_list" ? "allow" : "block") +
      '</td></tr>');
    bindCheckBox();

  });
};

/*******************************************************/

function bindCheckBox() {
  $('.select_appended').change(function () {
    if ($(this).is(':checked')) {
      $(this).closest('tr').addClass('selected');
    }
    else {
      $(this).closest('tr').removeClass('selected');
    }
  });
};

/*******************************************************/

function collectAppendedRules() {
  var appended_rules=[];
  // for each appended rule
  $(".manual_appeded").each(function () {
    // get a type of rule
    var table_id = $(this).closest('table').attr('id');
    var target = $(this).find("td.appended_rule_target")[0];
    var tr_id = $(this).attr('id');
    // put all field values to array
    var inputs =$(this).find("input.input_appended");
    var fields =[];
    inputs.each(function(i,el){
        var name=$(el).attr('name');
        var val=$(el).val();
        fields.push({[name]:val});
    });
    // put rule to the result array (appended_rules)
    appended_rules.push({table_id:table_id,tr_id:tr_id,target:$(target).text(),fields_arr:fields});
  });
  return appended_rules;
}

/*******************************************************/

// save deleted rules to local storage
function SaveDeleted(table_id) {
  var deletedFields = JSON.parse(localStorage.getItem('deletedFields')) || [];
  $('#' + table_id.data + " tr.selected").each(function () {
    var val = $(this).find("td > span[name='name']").text();
    if (val !== "") {
      deletedFields.push(val);
    }
    $(this).remove();
  });
  localStorage.setItem('deletedFields', JSON.stringify(deletedFields));
}