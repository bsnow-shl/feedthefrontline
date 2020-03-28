<?php

require_once 'db.h';
require_once 'form.h';
require_once 'phpshould.h';

#--------------------------------------------------
# create a simple html table from an sql table

#returns html to display the table specified in sql
#stick this in your .html file anywhere at all.
#  echo insert_display_table("select blah from blahblah where condition"); 
#no interaction with the .data file is necessary
function insert_display_table($sql) {
  $query = db_multirow($sql);
  $header = array();
  $result = "<table class=formdisplay>\n";
  foreach ($query as $row) {
    if (count($header) == 0) {
      $result .= "<tr>";
      foreach (array_keys($row) as $heading) {
	$result .= "<th>".form_beautify_identifier($heading)."</th>";
	array_push($header,$heading);
      }
      $result .= "</tr>\n";
    }
    $result .= "<tr>";
    foreach ($header as $next) {
      $result .= "<td>".$row[$next]."</td>";
    }
    $result .= "</tr>\n";
  }
  $result .= "</table>\n";

  return $result;
}

#--------------------------------------------------
# relatively straightforward edit of an sql table, in an html table/form

function insert_table_sql($sql) {
  return insert_table(db_multirow($sql),sqltablename($sql));
}

#call this method for the table placement from the html file
#$name is the form name and (maybe) the table name
#  it MUST match the form name in build_form_table
#depends on a call to build_form_table in the .data file.  Without it
#  all the cells will be empty
# Insert the following in the .html file where the edit table should appear
# form_start('table_name'); 
# echo insert_table($query,'table_name');
# form_end();
function insert_table($query, $name) {
  $result = "";
  $result .= "<table class=formedit>\n";
  $header = array();

  #ignore the data, and set up a form for the given metadata
  $rowcount = 1;
  foreach ($query as $row) {
    if (count($header) == 0) {
      $result .= "<tr>";
      foreach (array_keys($row) as $heading) {
	$result .= "<th>".form_beautify_identifier($heading)."</th>";
	array_push($header,$heading);
      }
      $result .= "</tr>\n";
    }

    $result .= "<tr>";
    foreach (array_keys($row) as $colname) {
      $result .= "<td>@@table_".$colname."_".$rowcount."@@</td>";
    }
    $result .= "</tr>\n";
    $rowcount = $rowcount +1;
  }
  $result .= "</table>\n";
  return $result;
}

//you can't use this with insert_table
function build_form_table($table) {
  build_form_table_sql("select * from ".$table);
}
//you can't use this with insert_table
function build_form_table_sql($sql) {
  build_form_table_array(db_multirow($sql),sqltablename($sql));
}

#return the word that follows "from" in $sql
function sqltablename($sql) {
  foreach (preg_split("/ +/",$sql) as $word) { # consider "/[^[:alpha:]]+/"
    if ($prev == "from") {
      return $word;
    }
    $prev = $word;
  }
}

#$query is more or less the result of db_multirow: a list of arrays
#$somewidgets should map db column names to 'form.h' widget data structures
#$keys is the list of uneditable columns
#$name is the form name, intended to be the same as the table name
#this method, which deals with both the 'form_set' and 'form_valid' parts of
#  form handling, requires a specially formatted table in the .html
#  file.  You can get it easily by calling insert_table with 
#  the same query as is used here.  For more careful presentation, consider
#  brewing the table yourself using a foreach on $query.  It's pretty easy,
#  and then you're not bound to the columns or rows in the query.
#  The table (or whatever) refers to editable elements as 
#  @@table_colname_rowcount@@ (this goes in the html), where colname is
#  (obviously) the column name, and rowcount is the count of rows, starting
#  at 1, in $query.  It has nothing to do with your primary key, because I
#  have no idea what that is.  You have to make a counter yourself in code.
#  table is actually table, and not your table name, in case you are confused
#  still.  What the heck, just look at insert_table!  You have to include 
#  the fields in $keys in @@ tags as well, or it will confuse the form
#  generator.
function build_form_table_array($query, $name, $keys = null, $somewidgets = array(), $anticolumns = array()) {
  #hold the widget map, to be built in lazy style:
  $allwidgets = array();
  #ultimately the parameter to form_set:
  $form = array();
  #rows in the table and form are identified by the following:
  $rowcount = 1;

  foreach ($query as $row) {

    #metadata processing
    if (count($allwidgets) == 0) {
      #build an array mapping column names to widgets, inclusing $somewidgets
      foreach (array_keys($row) as $column) {
	#more metadata processing: 
	#  if there are no $keys selected, then we have to use the first row
	if ($keys == null) {
	  $keys = array($column);
	}

	#continue by processing each column
	if (in_array($column,$keys)) {
	  $allwidgets[$column] = array('type' => 'label');
	} else if ($somewidgets[$column]) {
	  $allwidgets[$column] = $somewidgets[$column];
	} else {
	  $allwidgets[$column] = array('type' => 'text');
	}
	#we are going to have to add 'default' separately to each widget copy
        # as we go through the data
      }
    }
      
    #deal with the cells in this row individually
    foreach ($row as $column => $value) {
      if (!in_array($column,$anticolumns)) {
        #simple copy function: $nextwidget = copy($allwidgets[$column])
	$nextwidget = array();
	foreach ($allwidgets[$column] as $widgetparm => $widgetval) {
	  $nextwidget[$widgetparm] = $widgetval;
	}
	$nextwidget['default'] = $value;
	$form['table_'.$column.'_'.$rowcount] = $nextwidget;
      }
    }
    
    $rowcount += 1;
  }

  $form["submit"] = array('type'=>'submit', 'default'=>'Update');
  form_set($name,$form); 

  #for form submission: update the db table row by row!
  if (form_valid($name)) {
    $rowcount = 1;
    foreach($query as $row) {
      #build an update hash out of the current row info
      $update_sql = "update $name set ";
      $comma = '';
      $andmarker = '';
      $whereclause = '';

      foreach ($row as $column => $value) {
	if (!in_array($column,$keys)) {
	  if ($value!=$_REQUEST['table_'.$column.'_'.$rowcount]) {
            #not a key, so it goes in the 'set' clause

	    $update_sql .= $comma;
            #set all available form values for this row update
	    $update_sql .= $column."="._db_quote($_REQUEST['table_'.$column.'_'.$rowcount]);
	    $comma = ',';
	  }
	} else {
	  #its a key, so it goes in the 'where' clause
	  if ($value) {
	    $whereclause .= $andmarker." ".$column."="._db_quote($value);
	    $andmarker = " and ";
	  } #else it is null, and not considered a good key:
	    #  more specifically, we can nott tell the difference between null and ''
	}
      }
      
      $update_sql .= " where ".$whereclause;

      #only send the update if at least one column has changed
      #  be conservative: never change all rows in the table
      if ($comma == ',' && $whereclause) {
	db_send($update_sql);
      }
      #TODO: error handling for a bad update

      $rowcount += 1; 
    }
    if ($_REQUEST['goback']) {
      redirect($_REQUEST['goback']);
    }
  }

}

#--------------------------------------------------
# automatic insert form generation/handling from table meta-data

#$table is the table to build the insert form for
#$sql is a one-row query from $table, or null
#$insert_hash_bonus maps columns you do not want as form fields to the values to be inserted
#  use "sequence=sequencename" as the insert_hash_bonus to insert a value from a sequence
#  use "skip" to skip the column completely, even though it appears in the sql
function simple_sql_insert_form_widgets(&$form, $table, $sql=null, $insert_hash_bonus = array(), $formname=null, $somewidgets = array(), $createform=true) {
  #see also: _create_simple_form, simple_sql_edit_form_widgets

  if ($formname == null) {
    $formname = $table;
  }

  if ($sql == null) {
    $sql = "select * from ".$table." limit 1";
  }

  $query = db_row($sql);
  foreach ($query as $column => $value) {
    if (!$insert_hash_bonus[$column]) {
      if (isset($somewidgets[$column])) { 
  	  $form[$column] = $somewidgets[$column];
      } else {
        $form[$column] = array('type'=>'text','pretty'=>form_beautify_identifier($column));
      }
    }
  }

  if ($createform) {
    $form['submit'] = array('type'=>'submit','default'=>'Add');
    form_set($formname,$form);
  }

  if (form_valid($formname)) {
    #result will contain only new sequence results
    $result = array();

    #submit all the available request parameters, if they are part of the table!
    $sqlhash = array();
    foreach (array_keys($query) as $column) {
      if (isset($insert_hash_bonus[$column])) {
	if ($insert_hash_bonus[$column] == "skip") {
	  //just skip the column
	} else if (substr($insert_hash_bonus[$column],0,9) == "sequence=") {
	  $sqlhash[$column] = db_newid(substr($insert_hash_bonus[$column],9));
	  array_push($result,$sqlhash[$column]);
	} else {
	  $sqlhash[$column] = $insert_hash_bonus[$column];
	}
      } else if (isset($_REQUEST[$column]) && $_REQUEST[$column]!="") {
	$sqlhash[$column] = $_REQUEST[$column];
      }
    }
    db_insert_hash($table, $sqlhash);

    if ($_REQUEST['goback']) {
      redirect($_REQUEST['goback']);
    }

    if (count($result)==1) {
      return array_pop($result);
    } else if (count($result) >1) {
      return $result;
    } #else return nothing
  }
}

#To use this one, call this function in your .data file, and put
# form_start($table); form_end(); in your .html file.  Or put them
# all in one file, what do I care?
function simple_sql_insert_form($table,$sql=null, $insert_hash_bonus = array(), $formname=null) {
  $form = array();
  simple_sql_insert_form_widgets($form,$table,$sql,$insert_hash_bonus, $formname);
}

#the recursive part of simple_sql_edit_form
function _create_simple_form(&$form, $somewidgets,$table,$where,$subform_prefix="") {
  $result = array();
  $whereclause = "";

  if (is_array($where)) {
    foreach ($where as $parameter => $value) {
      if ($whereclause == "") {
        $whereclause = " where ";
      } else {
	$whereclause .= " and ";
      }
      $whereclause .= $parameter."="._db_quote($value);
    }
  } else if ($where) {
    $whereclause = "where ".$where;
  } 

  $query = db_row("select * from $table $whereclause");
  if (!$query) {
    die("Failed query: select * from $table $whereclause");
  }
  $result[$subform_prefix] = array('query'=>$query,'whereclause'=>$whereclause);

  foreach ($query as $column => $value) {
    if (isset($somewidgets[$column])) { 
      if ($somewidgets[$column] == "") {
	//skip it

      } else if ($somewidgets[$column]['type'] == 'inlineref') {
	//recurse
	$refcolumn = $column;
	if ($somewidgets[$column]['refcolumn']) {
	  $refcolumn = $somewidgets[$column]['refcolumn'];
	}
	$nextwhere = array($refcolumn=>_db_quote($value));
	$newprefix = $subform_prefix.$column."_";
	$subquery = _create_simple_form($form,$somewidgets[$column]['somewidgets'],$somewidgets[$column]['table'],$nextwhere,$subform_prefix.$column."_");
	$result = array_merge($result,$subquery);

      } else if ($somewidgets[$column]['type'] == 'ref') {
	//hyperlink
	$refcolumn = $column;
	if ($somewidgets[$column]['refcolumn']) {
	  $refcolumn = $somewidgets[$column]['refcolumn'];
	}
	$form[$subform_prefix.$column] = array('type'=>'label',
			       'default'=>"<a href=\"../data-admin/updaterow.html?table=".$somewidgets[$column]['table']."&col_$refcolumn="._db_quote($value)."\">$value</a>");

      } else {
	$form[$subform_prefix.$column] = array_merge(
	  array('default'=>$value,'pretty'=>form_beautify_identifier($column)),
	  $somewidgets[$column]
	  );
      }
    } else if (is_array($where) && $where[$column]) {
      $form[$subform_prefix.$column] = array('type'=>'label','default'=>$value,'pretty'=>form_beautify_identifier($column));
    } else {
      $form[$subform_prefix.$column] = array('type'=>'text', 'default'=>$value,'pretty'=>form_beautify_identifier($column));
    }
  }

  return $result;
}

function _interpret_simple_form($form,$queries,$request,$somewidgets,$table, $subform_prefix="") {
    $updatesql = "";
    foreach (array_keys($queries[$subform_prefix]['query']) AS $c) {
      #is it a recursive case?
      if ($somewidgets[$c]['type'] == 'inlineref') {
	#recurse
	_interpret_simple_form($form,$queries, $request, $somewidgets[$c]['somewidgets'], $somewidgets[$c]['table'], $subform_prefix.$c."_");
      } else {
	#build up the request for the single query data
	#check that data is there, and that it changed
	if (isset($request[$subform_prefix.$c]) && $form[$subform_prefix.$c]['default']!=$request[$subform_prefix.$c]) {
	  if ($updatesql != "") {
	    $updatesql .= ',';
	  }
	  $updatesql .= $c.'='._db_quote($request[$subform_prefix.$c]);
	}
      }
    }
    if ($updatesql != "") {
      #echo "<P>UPDATE $table SET $updatesql ".$queries[$subform_prefix]['whereclause'];
      db_send("UPDATE $table SET $updatesql ".$queries[$subform_prefix]['whereclause']);
    } #else there is nothing to update
}

function simple_sql_edit_form($table,$where=null, $somewidgets=null, $formname=null) {
  $form = array();
  simple_sql_edit_form_widgets($form,$table,$where,$somewidgets,$formname);
}

#same function as above, but for an edit form
#$where is an array of column=>value keys for the sql where clause
function simple_sql_edit_form_widgets(&$form, $table,$where=null, $somewidgets=null, $formname=null, $createform=true) {

  if ($formname == null) {
    $formname = $table;
  }

  $queries = _create_simple_form($form, $somewidgets,$table,$where);

  if ($createform) {
    $form['submit'] = array('type'=>'submit','default'=>'Update');
    form_set($formname,$form);
  }

  if (form_valid($formname)) {
    db_send("BEGIN");
    _interpret_simple_form($form,$queries,$_REQUEST,$somewidgets,$table);
    db_send("COMMIT");

    if ($_REQUEST['goback']) {
      redirect($_REQUEST['goback']);
    }
  }
}


?>
