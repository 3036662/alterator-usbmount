function InitUi(){
    // click events
    BindRowSelect();
    BindDoubleClick();
    // button click events
    BinEditRow(); 
    BindReset(); 
    BindDeleteSelectedRows();
    BindRowCheckbox();
}

// ------------------- rules list -------------------------
function UpdateRulesList(data){
    try{
        let json_arr=JSON.parse(data);
        let table = document.getElementById('rules_list_table');
        localStorage.setItem('rules_data',data);
        ClearList(table);
        for (index in json_arr){
            AppendTheRule(json_arr[index],index);
        }
        BindTableCheckbox(table);    
        BindDoubleClick();
        BindRowSelect();
        BindRowCheckbox();
    } catch(e){
        console.log(e.message);
    }
}

function ResetRulesList(){
    try{
        let data=localStorage.getItem('rules_data');
        let json_arr=JSON.parse(data);
        let table = document.getElementById('rules_list_table');
        ClearList(table);
        for (index in json_arr){
            AppendTheRule(json_arr[index],index);
        }    
        BindTableCheckbox(table);
        BindDoubleClick();
        BindRowSelect();
        BindRowCheckbox();
    } catch(e){
        console.log(e.message);
    }
}

// clear list;
function ClearList(table){
    let rows = table.getElementsByTagName('tr');
    while (rows.length > 1) {
        table.deleteRow(1);
    }
}

// append rule from json object
function AppendTheRule(item,index){
    const table = document.getElementById('rules_list_table');
    const tBody=table.getElementsByTagName('tbody')[0];
    let new_row =tBody.insertRow(-1);    
    index % 2 == 0 ? new_row.classList.add("tr_even"):new_row.classList.add("tr_odd");
    let checkbox_cell=new_row.insertCell(-1);
    let checkbox=document.createElement('input');
    checkbox.classList.add('list_checkbox');
    checkbox.setAttribute('type', 'checkbox');
    checkbox_cell.appendChild(checkbox);
    checkbox_cell.classList.add('padding_td');
    InsertCell(new_row,item.id,'rule_id');
    InsertCell(new_row,item.perm.device.vid,'rule_vid');
    InsertCell(new_row,item.perm.device.pid,'rule_pid');
    InsertCell(new_row,item.perm.device.serial,'rule_serial');
    InsertCell(new_row,item.perm.users[0].name,"rule_user");
    InsertCell(new_row,item.perm.groups[0].name,"rule_group");  
}

function InsertCell(row,text,htmlclass){
    let cell =row.insertCell(-1);
    cell.classList.add('padding_td');
    if (htmlclass) cell.classList.add(htmlclass);
    let text_span=document.createElement('span');
    let cell_text =document.createTextNode(text);
    text_span.appendChild(cell_text);
    if (htmlclass) text_span.classList.add(htmlclass+'_val');
    text_span.classList.add('span_val');
    cell.appendChild(text_span);
}

// ------------------ bind events ----------------------

// single select for table rows
function BindRowSelect(){
    let table = document.getElementById('rules_list_table');
    table.addEventListener('click',function(event){
        const clicked_row=event.target.closest('tr');
        if (!clicked_row || clicked_row.parentElement.nodeName!='TBODY') return;
        const rows = table.tBodies[0].querySelectorAll('tr');
        rows.forEach(row => {
            if (row.classList.contains('tr_selected')) {
                row.classList.remove('tr_selected');
            }
        });
        EnableButton(document.getElementById('reset_rule_btn'));
        clicked_row.classList.toggle('tr_selected');
        if (!clicked_row.classList.contains('editing') && !clicked_row.classList.contains('tr_deleted_rule'))
            EnableButton(document.getElementById("edit_rule_btn"));        
    });
}

// multiselect checkbox in the header of a table
function BindTableCheckbox(table){
    if (!table) return;
    try{
        let header_checkbox=table.tHead.querySelector('input.list_checkbox');
        header_checkbox.checked=false;
        header_checkbox.addEventListener('change',function(e){
            let  checkbox=e.target;
            if (!checkbox) return;
            let inputs;
            try{
                table=checkbox.parentElement.parentElement.parentElement.parentElement;
                let body= table.tBodies[0];                
                inputs=body.querySelectorAll('input.list_checkbox');

            }
            catch (e){
                console.log(e.message);
            } 
            if (!inputs) return;
            const event=new Event('change');          
            inputs.forEach(el=>{
                    el.checked=checkbox.checked;
                    el.dispatchEvent(event);
            });                
        });
    }
    catch (e){
        console.log(e.message);
    }    
}

// enable "delete" button if some rows are checked, disable if nothing is "checked"
function BindRowCheckbox(){
    let table = document.getElementById('rules_list_table');
    try{
        let checkboxes=table.tBodies[0].querySelectorAll('input.list_checkbox');
        if (!checkboxes) return;
        checkboxes.forEach(checkbox=>{
            checkbox.addEventListener('change',function(e){
                // if checked - enable button delete
                if (e.target.checked==true){
                    let delete_btn=document.getElementById('delete_rule_btn');
                    if (delete_btn.disabled==true) EnableButton(delete_btn);
                }
                // if not checked, check all rows, if nothing is checked - disable button
                else if (e.target.checked==false){
                    try{
                        let tbody=e.target.parentElement.parentElement.parentElement;
                        if (!tbody || tbody.nodeName!="TBODY") return;
                        let inputs=tbody.querySelectorAll('input.list_checkbox');
                        let some=false;
                        for (let i=0;i<inputs.length;++i){
                            if (inputs[i].checked){
                                some=true;
                                break;
                            }
                        }
                        // if nothing is checked
                        if (!some) {
                             DisableButton(document.getElementById('delete_rule_btn'));
                             // uncheck a checkbox in the header
                             document.getElementById('rules_list_table').tHead.querySelector('input.list_checkbox').checked=false;
                        }    
                    }
                    catch (e){ 
                        console.log(e);
                        return;
                    }
                }


            });
        });
    }
    catch (e){
        console.log(e.message);
    }
}

function DisableButton(button){
    if (!button) return;
    button.disabled=true;
    button.classList.add('like_btn_disabled');
    button.classList.remove('like_btn');
}

function EnableButton(button){
    if (!button) return;
    button.disabled=false;
    button.classList.remove('like_btn_disabled');    
    button.classList.add('like_btn');    
}

// bind boudle-clicks on table cells
function BindDoubleClick(){
     // double click on user
     let table = document.getElementById('rules_list_table');
     let user_cells= table.querySelectorAll('td.rule_user');
     user_cells.forEach(cell => {
         cell.addEventListener('dblclick',function(e){DblClickOnUserOrGroup(e,'users_list');});
     });
     // double click on groups
     let group_cells=table.querySelectorAll('td.rule_group');
     group_cells.forEach(cell => {
         cell.addEventListener('dblclick',function(e){DblClickOnUserOrGroup(e,'groups_list'); });
     });

     // double click on vid
     let vid_cells =table.querySelectorAll('td.rule_vid');
     vid_cells.forEach(cell => {
        cell.addEventListener('dblclick',DblClickOnEditable)
     });
     // double click on pid
     let pid_cells =table.querySelectorAll('td.rule_pid');
     pid_cells.forEach(cell => {
        cell.addEventListener('dblclick',DblClickOnEditable)
     });
     // double click on serial
     let serial_cells =table.querySelectorAll('td.rule_serial');
     serial_cells.forEach(cell => {
         cell.addEventListener('dblclick',DblClickOnEditable)
      });

}



// edit row button
function BinEditRow(){    
    let button=document.getElementById('edit_rule_btn');
    DisableButton(button);
    button.addEventListener('click',function(e){
        let table = document.getElementById('rules_list_table');
        let selected_row=table.querySelector('tr.tr_selected');
        if (selected_row){
            // edit the row if not deleted
            if (!selected_row.classList.contains("tr_deleted_rule"))
                MakeTheRowEditable(selected_row);
        }
    });
}; 

// delete rules button
function BindDeleteSelectedRows(){
    let button=document.getElementById('delete_rule_btn');
    DisableButton(button);
    button.addEventListener('click',function(e){
        let table = document.getElementById('rules_list_table');
        try {
            let checkboxes=table.tBodies[0].querySelectorAll('input.list_checkbox');
            checkboxes.forEach(checkbox=>{
                if (!checkbox.checked) return;
                let tr=checkbox.parentElement.parentElement;            
                // if a selected row is going to be deleted, disable the "edit" button
                if (tr.classList.contains('tr_selected')) DisableButton(document.getElementById('edit_rule_btn'));                
                ResetRow(tr); // reset if some change were made                
                if (tr && tr.nodeName=="TR") tr.classList.add('tr_deleted_rule'); // mark as deleted
            });

        }
        catch (e){
            console.log(e.message);
            return;
        }
    });
}


// ------------------- local data ---------------------

// Save lists of possible users and groups to the local storage
function SetUsersAndGroups(data){
    try{
       let json_obj=JSON.parse(data);
       localStorage.setItem("users_list",JSON.stringify(json_obj.users));
       localStorage.setItem("groups_list",JSON.stringify(json_obj.groups));            
    }
    catch(e){
        console.log(e.message);
    }
}

function BindReset(){
    let button=document.getElementById('reset_rule_btn');
    DisableButton(button);
    // reset a row
    button.addEventListener('click',function(e){
        let table = document.getElementById('rules_list_table');
        let selected_row=table.querySelector('tr.tr_selected');   
        if (selected_row){
            ResetRow(selected_row);
        }
    });
    // reset all
    let button_all=document.getElementById('reset_all_btn');
    button_all.addEventListener('click',function(e){
        ResetRulesList();
        DisableButton(document.getElementById('edit_rule_btn'));
    });

}

// ---------------------- Select user or group -----------------------------

function DblClickOnUserOrGroup(event,storage_name){
    let td_el;
    let span_el;
    if (event.target.nodeName=="TD"){
        td_el=event.target;
        span_el=td_el.querySelector('span.span_val');
    } else if(event.target.nodeName=="SPAN") {
        span_el=event.target;
        td_el=span_el.parentElement;
    }
    if (span_el)
        span_el.classList.add('hidden');
    if (td_el){
        let dropdown=CreateUserGroupSelect(storage_name);
        BindRemoveSelectOnFocusLost(dropdown);
        td_el.classList.toggle('padding_td');
        td_el.appendChild(dropdown);
        dropdown.focus();
    }
}


function CreateUserGroupSelect(storage_name){
    const stored_items = localStorage.getItem(storage_name);
    const items = stored_items ? JSON.parse(stored_items) : [];
    let dropdown = document.createElement("select");
    const empty_option = document.createElement('option');
    empty_option.text = "";
    empty_option.value="-";
    dropdown.add(empty_option);
    items.forEach(item => {
        const option = document.createElement('option');
        option.text = item.name;
        option.value=item.name;
        dropdown.add(option);
    });
    dropdown.classList.add('edit_inline');
    return dropdown;
}

// bind event on focus out - if nothing was chosen, remove select
function BindRemoveSelectOnFocusLost(select_element){
    select_element.addEventListener("focusout", (event) => {
             SaveSelectValueToSpan(event.target);   
    });
    select_element.addEventListener('keyup',(event)=>{
        if (event.keyCode==27 ) // escape
            SaveSelectValueToSpan(event.target,true);               
        if (event.keyCode==13) // enter
            SaveSelectValueToSpan(event.target);  
    });

}

function SaveSelectValueToSpan(select,do_not_save){
    if (!select.parentElement) return;
    let sibling_span=select.parentElement.querySelector('span.span_val');
    let parent_td=select.parentElement;
    if (!sibling_span || !parent_td || parent_td.nodeName!='TD') return;    
    if (!do_not_save && select.value && select.value!='-' && select.value!=sibling_span.textContent){
        sibling_span.textContent=select.value;
        parent_td.classList.add('td_value_changed');
    }    
    parent_td.classList.toggle('padding_td');
    select.remove();
    sibling_span.classList.remove('hidden');
}

// ------------- edit vid,pid or serial ----------------------

function DblClickOnEditable(event){
    let td_el;
    let span_el;
    if (event.target.nodeName=="TD"){
        td_el=event.target;
        span_el=td_el.querySelector('span.span_val');
    } else if(event.target.nodeName=="SPAN") {
        span_el=event.target;
        td_el=span_el.parentElement;
    }
    if (span_el)
        span_el.classList.add('hidden');
    if (td_el){
        let input=CreateInput(span_el.textContent);
        BindRemoveInputOnFocusLost(input);
        td_el.classList.toggle('padding_td');
        td_el.appendChild(input);
        input.focus();
    }
}

function CreateInput(initial_text){
    let input = document.createElement("input");
    input.value=initial_text;
    input.classList.add("edit_inline");
    return input;
}

function BindRemoveInputOnFocusLost(input){
    // add event on focus out - if nothing was chosen, remove select
    input.addEventListener("focusout", (event) => {
        SaveInputValueToSpan(event.target);               
    });
    input.addEventListener('keyup',(event)=>{
        if (event.keyCode==27 )
            SaveInputValueToSpan(event.target,true);  
        if (event.keyCode==13)
            SaveInputValueToSpan(event.target);                 
    });
}

function SaveInputValueToSpan(input,do_not_save){
    let parent_td=input.parentElement;
    if (!parent_td) return;
    let initial_text;
    let sibling_span=input.parentElement.querySelector('span.span_val');
    if (sibling_span) initial_text=sibling_span.textContent;
    if (!sibling_span || !initial_text || !parent_td || parent_td.nodeName!='TD') return;
    if (!do_not_save && input.value!="" && input.value!=sibling_span.textContent){
        sibling_span.textContent=input.value;
        parent_td.classList.add('td_value_changed');
    }
    parent_td.classList.toggle('padding_td');
    input.remove();
    sibling_span.classList.remove('hidden'); 
}

// ------------------- edit row -------------

function ResetRow(row){
    let td=row.querySelector('td.rule_id');
    if (!td) return;
    row.classList.remove('tr_deleted_rule');
    let val_span=td.querySelector('span.span_val');
    if (!val_span) return;
    let rule_id=val_span.textContent;
    try {
        let data=localStorage.getItem("rules_data");
        if (!data) return;
        let rules_arr=JSON.parse(data);
        let rule_obj=rules_arr.find(obj => obj.id=== rule_id);
        if (!rule_obj) return;
        //vid 
        let vid_span=row.querySelector('td.rule_vid').querySelector('span.span_val');
        if (vid_span) vid_span.textContent=rule_obj.perm.device.vid;
        let  pid_span=row.querySelector('td.rule_pid').querySelector('span.span_val');
        if (pid_span) pid_span.textContent=rule_obj.perm.device.pid;
        let  serial_span=row.querySelector('td.rule_serial').querySelector('span.span_val');
        if (serial_span) serial_span.textContent= rule_obj.perm.device.serial;
        let user_span=row.querySelector('td.rule_user').querySelector('span.span_val');
        if (user_span) user_span.textContent = rule_obj.perm.users[0].name;
        let group_span = row.querySelector('td.rule_group').querySelector('span.span_val');
        if (group_span) group_span.textContent =rule_obj.perm.groups[0].name;
        let td_elements=row.querySelectorAll('td');
        td_elements.forEach(td => {
            td.classList.remove('td_value_changed');
        });
    }
    catch (e){
        console.log(e.message);
    }
}

// make the whole row editable
function MakeTheRowEditable(row){
    if (row.classList.contains('editing')) return;
    row.classList.add('editing');
    const td_elements = row.querySelectorAll('td');
    td_elements.forEach(td=>{
        let td_class;
        if (td.classList.contains('rule_vid')) td_class='rule_vid';
        else if (td.classList.contains('rule_pid')) td_class='rule_pid';
        else if (td.classList.contains('rule_serial')) td_class='rule_serial';
        else if (td.classList.contains('rule_user')) td_class='rule_user';
        else if (td.classList.contains('rule_group')) td_class='rule_group';
        // create inputs
        const input_classes= ['rule_vid', 'rule_pid', 'rule_serial'];
        let is_input=input_classes.some(class_name => td.classList.contains(class_name));
        let span_el=td.querySelector('span.span_val');
        if (is_input && span_el){
                let input =CreateInput(span_el.textContent);
                BindRowFocusLost(input);
                td.classList.toggle('padding_td');    
                span_el.classList.toggle('hidden');
                td.appendChild(input);
                // focus on first field
                if (td_class==='rule_vid') input.focus();
        }
        // create selects
        const select_classes = ['rule_user','rule_group'];
        let is_select=select_classes.some(class_name => td.classList.contains(class_name));
        if (is_select && span_el){
            let storage;
            if (td_class=="rule_user") storage='users_list';
            else if (td_class=="rule_group") storage='groups_list';
            if (storage){
                let select=CreateUserGroupSelect(storage);
                BindRowFocusLost(select);
                td.classList.toggle('padding_td');
                span_el.classList.toggle('hidden');
                td.appendChild(select);
            }
        }
    });
    DisableButton(document.getElementById("edit_rule_btn"));
}

// if all inputs in row lost their focus, save values, remove inputs
function BindRowFocusLost(input){
    input.addEventListener("focusout",(event)=>{
        let row_has_focus=false;
        let row=event.target.parentElement.parentElement;
        if (row.nodeName!='TR') return;
        let td_elements=row.querySelectorAll('td');
        td_elements.forEach(td=>{
            let input=td.querySelector('input');
            let select=td.querySelector('select');
            if (input && input==event.relatedTarget) row_has_focus=true;
            if (select && select==event.relatedTarget) row_has_focus=true;    
        });
        // if row has no focus -save values and remove inputs and selects
        if (row_has_focus) return;
        td_elements.forEach(td=>{
            let curr_input=td.querySelector('input');
            let curr_select=td.querySelector('select');
            if (!curr_input && !curr_select) return; 
            if (curr_input)
                SaveInputValueToSpan(curr_input);
            else if (curr_select)
                SaveSelectValueToSpan(curr_select);              
        });
        row.classList.remove('editing');
        EnableButton(document.getElementById("edit_rule_btn"));    
    });
    input.addEventListener('keyup',(event)=>{
        if (event.keyCode==27){
            let row=event.target.parentElement.parentElement;
            if (row.nodeName!='TR') return;
            let td_elements=row.querySelectorAll('td');
            td_elements.forEach(td=>{
                let curr_input=td.querySelector('input');
                let curr_select=td.querySelector('select');
                if (!curr_input && !curr_select) return; 
                if (curr_input)
                    SaveInputValueToSpan(curr_input,true);
                else if (curr_select)
                    SaveSelectValueToSpan(curr_select,true);              
            });
            row.classList.remove('editing');
            EnableButton(document.getElementById("edit_rule_btn"));      
        }
    });


  
}