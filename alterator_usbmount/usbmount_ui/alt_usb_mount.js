function InitUi(){
    BindRowSelect();
    BindDoubleClick();
    BinEditRow();  
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
    // insert a pen picture
    // let cell=new_row.insertCell(-1);
    // let svg=document.createElementNS("http://www.w3.org/2000/svg", "svg");
    // svg.setAttribute("xmlns", "http://www.w3.org/2000/svg");
    // svg.setAttribute("viewBox", "0 0 0.72 0.72");
    // svg.setAttribute("width", "24");
    // svg.setAttribute("height", "24");
    // const path = document.createElementNS("http://www.w3.org/2000/svg", "path");
    // path.setAttribute('d',"M0.659 0.217A0.036 0.036 0 0 0 0.652 0.197L0.523 0.068A0.036 0.036 0 0 0 0.503 0.061a0.036 0.036 0 0 0 -0.02 0.008l-0.085 0.084 -0.329 0.328A0.036 0.036 0 0 0 0.06 0.502v0.128A0.036 0.036 0 0 0 0.09 0.66h0.128A0.036 0.036 0 0 0 0.241 0.653L0.568 0.325 0.652 0.24 0.658 0.23V0.22ZM0.205 0.6H0.12V0.515L0.418 0.217l0.085 0.085ZM0.546 0.259 0.461 0.174 0.504 0.133l0.085 0.085Z");
    // svg.appendChild(path);
    // cell.appendChild(svg);
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
        let button=document.getElementById("edit_rule_btn");
        if (button){
            button.disabled=false;
            button.classList.remove('like_btn_disabled');    
            button.classList.add('like_btn');        
        }

    });
}

// edit row button
function BinEditRow(){    
    let button=document.getElementById('edit_rule_btn');
    button.disabled=true;
    button.classList.add('like_btn_disabled');
    button.classList.remove('like_btn');
    button.addEventListener('click',function(e){
        let table = document.getElementById('rules_list_table');
        let selected_row=table.querySelector('tr.tr_selected');
        if (selected_row){
            // TODO open row editor
            alert("click edit");
        }
    });
}; 


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

// ------------------- edit row -------------



