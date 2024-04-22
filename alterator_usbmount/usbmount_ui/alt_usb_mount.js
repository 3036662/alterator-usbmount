function InitUi(){
    BindRowSelect();
    BindDoubleClick();  
}

// ------------------- rules list -------------------------

function UpdateRulesList(data){
    try{
        let json_arr=JSON.parse(data);
        let table = document.getElementById('rules_list_table');
        ClearList(table);
        for (index in json_arr){
            AppendTheRule(json_arr[index],index);
        }    
    } catch(e){
        console.log(e.message);
    }
    BindDoubleClick();
    BindRowSelect();
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
    InsertCell(new_row,item.id);
    InsertCell(new_row,item.perm.device.vid,'rule_vid');
    InsertCell(new_row,item.perm.device.pid,'rule_pid');
    InsertCell(new_row,item.perm.device.serial,'rule_serial');
    InsertCell(new_row,item.perm.users[0].name,"rule_user");
    InsertCell(new_row,item.perm.groups[0].name,"rule_group");  
}

function InsertCell(row,text,htmlclass){
    let cell =row.insertCell(-1);
    if (htmlclass) cell.classList.add(htmlclass);
    let text_span=document.createElement('span');
    let cell_text =document.createTextNode(text);
    text_span.appendChild(cell_text);
    if (htmlclass) text_span.classList.add(htmlclass+'_val');
    cell.appendChild(text_span);
}

// ------------------ bind events ----------------------

// single select for table rows
function BindRowSelect(){
    let table = document.getElementById('rules_list_table');
    table.addEventListener('click',function(event){
        const rows = table.querySelectorAll('tr');
        rows.forEach(row => {
            if (row.classList.contains('tr_selected')) {
                row.classList.remove('tr_selected');
            }
        });
        const clicked_row=event.target.closest('tr');
        if (clicked_row) clicked_row.classList.toggle('tr_selected');
    });
}

// bind boudle-clicks on table cells
function BindDoubleClick(){
     // double click on user
     let table = document.getElementById('rules_list_table');
     let user_cells= table.querySelectorAll('td.rule_user');
     user_cells.forEach(cell => {
         cell.addEventListener('dblclick',function(e){DblClickOnUserOrGroup(e,'rule_user_val','users_list');});
     });
     // double click on groups
     let group_cells=table.querySelectorAll('td.rule_group');
     group_cells.forEach(cell => {
         cell.addEventListener('dblclick',function(e){DblClickOnUserOrGroup(e,'rule_group_val','groups_list'); });
     });

     // double click on vid
     let vid_cells =table.querySelectorAll('td.rule_vid');
     vid_cells.forEach(cell => {
        cell.addEventListener('dblclick',function(e){DblClickOnEditable(e,'rule_vid_val');})
     });
     // double click on pid
     let pid_cells =table.querySelectorAll('td.rule_pid');
     pid_cells.forEach(cell => {
        cell.addEventListener('dblclick',function(e){DblClickOnEditable(e,'rule_pid_val');})
     });
     // double click on serial
     let serial_cells =table.querySelectorAll('td.rule_serial');
     serial_cells.forEach(cell => {
         cell.addEventListener('dblclick',function(e){DblClickOnEditable(e,'rule_serial_val');})
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

// ---------------------- Select user or group -----------------------------

function DblClickOnUserOrGroup(event,span_class,storage_name){
    let td_el;
    let span_el;
    if (event.target.nodeName=="TD"){
        td_el=event.target;
        span_el=td_el.querySelector('span.'+span_class);
    } else if(event.target.nodeName=="SPAN") {
        span_el=event.target;
        td_el=span_el.parentElement;
    }
    if (span_el)
        span_el.classList.add('hidden');
    if (td_el){
        let dropdown=CreateUserGroupSelect(span_class,storage_name);
        td_el.appendChild(dropdown);
        dropdown.focus();
    }
}


function CreateUserGroupSelect(span_class,storage_name){
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
        if (storage_name=="users_list")
            option.value=item.uid;
        else if (storage_name=="groups_list")
            option.value=item.gid;
        dropdown.add(option);
    });
    // add event on focus out - if nothing was chosen, remove select
    dropdown.addEventListener("focusout", (event) => {
        if (!event.target.value || event.target.value==='-'){
            // show a sibling span
            event.target.parentElement.querySelector('span.'+span_class).classList.remove('hidden');
            // remove select
            event.target.remove();
        }
    });
    return dropdown;
}

// ------------- edit vid,pid or serial ----------------------

function DblClickOnEditable(event,span_class){
    let td_el;
    let span_el;
    if (event.target.nodeName=="TD"){
        td_el=event.target;
        span_el=td_el.querySelector('span.'+span_class);
    } else if(event.target.nodeName=="SPAN") {
        span_el=event.target;
        td_el=span_el.parentElement;
    }
    if (span_el)
        span_el.classList.add('hidden');
    if (td_el){
        let input=CreateInput(span_class,span_el.textContent);
        td_el.appendChild(input);
        input.focus();
    }
}

function CreateInput(span_class,initial_text){
    let input = document.createElement("input");
    input.value=initial_text;
    // add event on focus out - if nothing was chosen, remove select
    input.addEventListener("focusout", (event) => {
        if (!event.target.value || event.target.value===initial_text){
            // show a sibling span
            event.target.parentElement.querySelector('span.'+span_class).classList.remove('hidden');
            // remove select
            event.target.remove();
        }
    });
    return input;
}



