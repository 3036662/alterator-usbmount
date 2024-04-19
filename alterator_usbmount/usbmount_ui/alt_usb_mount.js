function InitUi(){
    BindRowSelect();
    BindDoubleClick();  
}

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
    InsertCell(new_row,item.perm.device.vid);
    InsertCell(new_row,item.perm.device.pid);
    InsertCell(new_row,item.perm.device.serial);
    InsertCell(new_row,item.perm.users[0].name,"rule_user");
    InsertCell(new_row,item.perm.groups[0].name);
    BindRowSelect();
    BindDoubleClick();
}

function InsertCell(row,text,htmlclass){
    let cell =row.insertCell(-1);
    if (htmlclass) cell.classList.add(htmlclass);
    let cell_text =document.createTextNode(text);
    cell.appendChild(cell_text);
}

// bind events
function BindRowSelect(){
    // single select for table rows
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

function BindDoubleClick(){
     // double click on user
     let table = document.getElementById('rules_list_table');
     let user_cells= table.querySelectorAll('td.rule_user');
     user_cells.forEach(cell => {
         cell.addEventListener('dblclick',DblClickOnUser);
     });
}

function DblClickOnUser(event){
    alert(event.target.textContent);
}

