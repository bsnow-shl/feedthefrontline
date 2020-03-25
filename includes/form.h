<?php

/**
The forms facility provides automatic, simple form generation and handling, using a compact language of form definition and validation.

Forms should be defined and validated in your part's .data file, and may be laid out in your part's .html file.

form_set() defines a form for the page.
form_start() and form_end() lay out a form
form_was_submitted() indicates if any form was submitted
form_submitted() indicates if a particular form was submitted
form_valid() indicates if a form meets all validation rules
form_error() records an error in a form, during custom validation code
form_add_note() makes a note on a form, often to indicate success or comments on a submission
*/

$_form_errors = $_form_notes = $_form_options = array();

require_once 'ckeditor.php';
require_once 'ckfinder_php5.php';


/**
Defines a form's widgets.

@usage:
- form_set(spec)
- form_set(name, spec)
- form_set(name, spec, defaults)
- form_set(name, spec, defaults, options)

@parameters:
- name: a string identifying the form, among others on the current page; if not provided, the form is placed under the identifier _default_.
- spec is a list of key-value pairs defining the widgets on the form. See below.
- defaults is an optional list of values for those widgets. This is much like setting the _default_ key on each widget in the spec, only easier.
- options controls special goodies in the form. Must be a hash.
  - contents_at_end: Ordinarily form_start() and form_end() bracket material and widgets which are shown above the automatically-rendered form widgets. This moves that stuff below the automatically-rendered widgets.

@returns nothing of interest

@example
  
  form_set(array(
	'name'=>'text,required',
	'age' => array('type'=>'integer', 'min' => 16),
	'gender' => array('type'=>'menu','radio'=>1, 'required'=>1, 'menu' => array(
		'm'=>'Male', 'f'=>'Female', 'na'=>'Not sure'
	))
  ));

The form spec is a series of key-value pairs, where the key is the identifier for the widget, and the value is the the widget descriptor.

The descriptor is hash, in which must _type_ must be set. In addition to _type_ a number of constraints and facts about the widget can be set.

Some constraints:
- required: almost every form element can specify this to insist that the form element be filled in, or (in the case of menus or radio buttons) have an item chosen.
- default: can be used to specify a default value for a widget if it hasn't been filled in yet.
- maxlength: constrains the maximum string length accepted by the browser, and returned by the form handling system, for freeform-entry fields.
- size: measured in em typographic units, specifies the apparent width of any given form element.
- min/max: for things that are numberish (dollars, integer), specifies the minimum or maximum values.  for this that are stringish (text,email), specifies the minimum and maximum lengths.
- positive: like min=1 for numberish widgets.
- hint: You an provide a helpful tip in 'hint' which will get shown in the third column of an automatic rendering.

Form element types:
- text: is a simple text input field.
- email: is equivalent to text, except it requires that the input look like an email address.
- urifragment: a text input field that wants only a piece of a URI, i.e. a-z 0-9 and maybe underscore.
- bigtext: is a big text input field, rendered as a textarea. The field defaults to 30x6, which can be modified by setting the rows and cols constraints.
- richtext: uses some crapware fancy-editing system to permit entry of styled text, and hopefully ward off skanky MS Word tag soup. Use the rows and cols constraints to adjust its size. Text is returned to your script with HTML tags.
- integer/float: is a plain text field that expects an integer or float. It will accept nothing (unless required is set), and will cast whatever's provided into the right type using PHP's loose rules.  "5 apples" becomes 5.
- dollars: similar to float, only tolerant of the crud humans put into dollar fields: "$5,000.111" becomes 5000.11
- date: accepts a wide variety of date specifications (e.g. "tomorrow", "next Monday", "2005-09-12") and returns a string of the form YYYY-MM-DD.
- timestamp: returns a string of the form YYYY-MM-DD HH:MM:SS.

- visible: This is used to render a field in the form that the user is not expected to be able to edit. For instance, if you're asking fora delivery address but you only deliver to Canada, you might say: 'country' => array('type'=>'visible', 'value'=> 'Canada')
- hidden: This hold-over from the old, bad web development days is deprecated. Use $_SESSION  instead.
- label: This is a inline form label that does not correspond to any particular data. It is rendered as a simple text line and does not result in values being POSTed to your form. 'password'=> 'password', 'note'=> array('type'=>'label', 'value'=>'Passwords should be at least 6 characters.');
- menu: renders a series of choices for the user. Several constraints influence their presentation. You can use multiple=1 to let the user choose several options; otherwise they can only choose one. You can use radio=1 to render the choice as radio buttons. If you use required=1, the user will be forced to choose one of the options (or at least one, in the case of a multiple-choice menu).

The contents of the menu itself are dictated by either the menu or sql constraints. If you use menu, you must provide a series of key/value pairs. The key is the internal identifier, and the value is shown to the user as the label. If you use sql, you must provide an SQL query that returns two columns, one titled key and the other value.

Example:
	form_set(array( 
		'gender' => array('type'=>'menu', 'radio' => 1, 'default'=>'P', 'required' => 1,
			'menu' => array( 'M'=>'male', 'F'=>'female', 'P'=>'prefer not to say')),
		'country' => array('type'=>'menu', 'required' => 1, 
			'sql'=>'select iso_code as key, name as value from countries ) 
		));

Radio-style menus have some choices for layout. By default, they are one per line.  Use inline to render them all on a single line, or set customlayout to indicate that each radio button is going to be laid out individually. See form_start() for more on that.
 
- file: Accepts a file upload off the user's machine. The file is placed under [server-root]/uploads/ to a filename given by the current timestamp (with milliseconds), and an entry is inserted into the uploads table with the file's size, mime type, and original filename. This sets several useful variables in your script's scope. Where 'foo' is the name of your file field:
  - $foo is the path (relative to the document root) to the permanent home of the file.
  - $foo_upload_id is the primary key to uploads
  - $foo_size is the size of the file
  - $foo_name is the original filename on the user's computer
  - $foo_mimetype is the mime type presented by the user's computer for the file.

If you don't want the file to survive past the end of your script, use upload_delete() to remove it.
*/
function form_set($p1, $p2 = '', $defaults=array(), $options = array()) {
	global $_form_contents, $_form_options;

	if ($p2) {
		$name = $p1;
		$widgets = &$p2;
	} else {
		$name = 'default';
		$widgets = &$p1;
	}

	$_form_options[$name] = $options;

	# promote each widget to an array
	foreach ($widgets AS $key=>$widget) {
		if (is_scalar($widget)) {
			$chunks = explode(',',$widget);
			if (sizeof($chunks)<=1) {
				$widgets[$key]=array('type'=>$chunks[0]);
			} else if(sizeof($chunks)==2) {
				$widgets[$key]=array('type'=>$chunks[0],
						     $chunks[1]=>1);
			} else {
				for ($i=0; $i<sizeof($chunks); $i+=2) {
					$widget[$key][$chunks[$i]] = $chunks[$i+1];
				}
			}
		} 
		if (isset($defaults[$key])) {
			$widgets[$key]['default'] = $defaults[$key];
		}

		if ($widgets[$key]['type'] == 'bigtext') {
			if (!$widgets[$key]['rows']) { 
				$widgets[$key]['rows'] = 6;
			} 
			if (!$widgets[$key]['cols']) {
				$widgets[$key]['cols'] = 40;
			}
		}
        if ($widgets[$key]['type'] == 'date') {
            static $added_foot=0;
            if (!$added_foot++) {
                global $_cpf_head_magic;
                global $_cpf_foot_magic;
                $_cpf_foot_magic .= '    <script> $(function() { $.datepicker.setDefaults( { dateFormat: "yy-mm-dd" } );  $( ".date" ).datepicker(); }); </script> ';
    ;
            }
        }
		if ($widgets[$key]['type'] == 'richtext') {
            if (!$added_richtext_head++) {
                    global $_cpf_head_magic;
                    $_cpf_head_magic .= "<script type=\"text/javascript\" src=\"/.ckeditor/ckeditor.js\"></script>";
            }
        }
	}
	$_form_contents[$name] = $widgets;

	if (form_was_submitted()==$name) {
		_form_handle_submission($name);
	}
}


/** Begins the layout of a form.  If you are happy with the automatic layout style of forms, follow this by a call to form_end() and forget about it.  

If you want to have a custom form layout, provide markup between the calls to form_start() and form_end().  These calls must not be nested.

Automatic rendering is described in form_end()'s documentation.

With manual rendering, you must specify where the form will display errors, and where it will place form widgets. A widget is manually placed by placing a @@token@@ where the widget should appear, and another ##token## where the widget's errors should appear.  _token_ is the key of the widget passed into form_set().

If you're doing a manual layout, you will want to include all of the widgets passed into form_set().  If you miss some, they will get automatically rendered onto the bottom of your manual layout, as a reminder that you've forgotten them.

You can also do manual layout of radio buttons if the client is picky about where individual buttons go.  If the radio button is named _gender_ and you have keys _m_ and _f_ for each of the choices, you can place them with the tokens @@gender.m@@ and @@gender.f@@.  Errors can be rendered for each radio button, or for the radio group as a whole with ##gender.f## or ##gender## respectively, with corresponding calls to form_error() needed to actually throw the errors.

@parameters:
- name: must match what was set (or defaulted) in the call to form_set()
- extra: extra sludge (usually javascript) to put into the <form> tag
- method: post or get, default post

@returns nothing useful
*/
function form_start($name = 'default', $extra='', $method='post') {
	global $_form_contents, $_form, $_form_notes, $_form_name;
	if (!is_array($_form_contents[$name])) {
		print "Never heard of the form '$name'.";
	}
	
	$_form = $_form_contents[$name];
	$_form_name = $name;	# caught in form_end
	
	echo "<form class=\"cpf_form\" enctype=\"multipart/form-data\" method=$method name=\"$name\" $extra><input type=hidden name=\"_form_id\" value=\"$name\" />";

	if ($_form_notes[$name]) {
		foreach ($_form_notes[$name] AS $ignored=>$n) {
			echo '<div class="formnote">'.$n.'</div>';
		}
	}
	ob_start();
}


/** Registers an error against one of the form widgets.  This function is usually called during the validation step:
	if (form_was_submitted()) {
		# maybe call form_error() here
	}

These errors are shown in the automatic re-rendering of the form, or substituted for ##tokens## in the manual rendering of the form.
	
@parameter widget the name of the widget that whose value you're not happy with
@parameter error the textual complaint
@return nothing
*/
function form_error($widget, $error) {
	global $_form_errors;
	$_form_errors[$widget] .= '<span class="oneerror">'.$error.'</span>  ';
}

/**
  Causes a note <div class="formnote"> to show up at the top of form (both manual and automatic renderings), for instance to indicate success.
*/
function form_add_note($note, $name = 'default') {
	global $_form_notes;
	$_form_notes[$name][] = $note;
}


function form_was_submitted() {
	if ($_SERVER['REQUEST_METHOD']=='GET' && $_GET['_cpf_treataspost']) {
		$_POST = $_GET;
		$_SERVER['REQUEST_METHOD'] = 'POST';
		unset($_POST['_cpf_treataspost']);
	}

	if ($_SERVER['REQUEST_METHOD'] != 'POST') { return null; }
	return $_REQUEST['_form_id'];
}

function form_submitted($which = 'default') {
    return (form_was_submitted() == $which);
}

/** Closes off the layout of a form, automatically rendering any widgets that were missing. 

Automatically-rendered forms appear as an HTML table, with three columns. The rows of the table are 1-1 with each key of the form declared to form_set(). The first column is the key in the array provided to form_set(), used as labels to prompt the user through a prettification algorithm that turns 'your_name' into 'Your name'. The second column is the form widgets, rendered according to their type and constraints. The third column is where any hints are shown, or errors if validation errors happened.
*/
function form_end() {
	global $_form, $_form_name, $_form_options;

	# looking up error messages in widgets:
	# if our form being POSTed, use the errors list
	# but if errors are being handled separately, don't bother for widgets.
	$widget_error_lookup = array();
	if (config_get('form', 'separate_errors')) {
		$separate = 1;
	}
	if (form_submitted($_form_name)) {
		global $_form_errors;
		$widget_error_lookup = $errors = &$_form_errors;
		if ($separate) {
			$widget_error_lookup = array();
		}
	}

	$contents = ob_get_contents();
	ob_end_clean();

	$matches = $unset_list = array();
	preg_match_all('/([#@^])\1([\[\]a-z_\-\.A-Z0-9]+)\1\1/', $contents, $matches);
	foreach ($matches[2] AS $counter=>$key) {
		if ($_form[$key]) {
			$widget_key = $key;
			$unset_list[] = $widget_key;
		} else {
			$widget_key = substr($key, 0, strpos($key,'.'));
			$unset_list[] = $widget_key;
		}
	
		if ($matches[1][$counter]=='@') {
			$contents = str_replace("@@$key@@", form__html_for_widget($key, $_form[$widget_key], $widget_error_lookup), $contents);			
			if ($_form[$widget_key]['type']=='submit') {
				$seen_submit = 1;
			}
		} else if ($matches[1][$counter]=='^') {
			$contents = str_replace("^^$key^^", $_form[$key]['pretty'], $contents);			
		} else {
            $err = form__error_for_widget($key);
            if (!$err) $err = '<span class="hint">'.$_form[$key]['hint'].'</span>';
            else $err = '<span class="error">'.$err.'</span>';
			$contents = str_replace("##$key##", $err, $contents);
		}
		
	}
	foreach ($unset_list as $k) {
		unset($_form[$k]);
	}
	if (!$_form_options[$_form_name]['contents_at_end']) { echo $contents; }
	if (sizeof($_form)) {
		echo '<table class="formedit">';
		foreach ($_form as $key=>$widget) {
			if ($widget['type']=='submit') {
				$seen_submit = 1;
			}
			if ($widget['type'] == 'hidden') {
				echo form__html_for_widget($key, $widget, $widget_error_lookup);
			} else {
				echo "\n<tr>";
				if (!isset($widget['pretty']) && $widget['type'] != 'submit' ) {
					$widget['pretty'] = beautify_identifier($key);
				}
				echo '<th align=right>'.($widget['required']?'<span class="required">':'<span class="notrequired">').$widget['pretty'].'</span></th><td>'.form__html_for_widget($key, $widget, $widget_error_lookup).'</td>';
				if ($separate) {
					if ($errors[$key]) {
						echo '<td><div class=error>'.$errors[$key].'</span></td>';
					} else {
						echo '<td><span class="hint">'.$widget['hint'].'</span></td>';
					}
				}

				echo '</tr>';
			}
		}
		if (!$seen_submit) {
			echo '<tr><td></td><td><input id="'.$_form_name.'_submit" type=submit value="'. _('Proceed'). '" /></td></tr>';
			$seen_submit = 1;
		}
		echo '</table>';
	}

	if (!$seen_submit) {
		echo '<input type=submit value="'. _('Proceed'). '" />';
	}
	
	if ($_form_options[$_form_name]['contents_at_end']) { echo $contents; }

	echo '</form>';
}

/** Returns a list of errors that were raised in a given form. Useful for logging. */
function form_errors($name = 'default') {
	if (!(form_was_submitted() == $name)) { return false; }
	global $_form_errors;
	return ($_form_errors);
}

/** Returns true if the given form has no errors on it. */
function form_valid($name = 'default') {
	if (!(form_was_submitted() === $name)) { return false; }

	global $_form_errors;
	return (empty($_form_errors));
}

/** Computes the base64 encoding of the named file's contents.  Only sensible in a block protected by form_valid().
*/
function form_base64($key) {
	$fh = fopen($_FILES[$key][tmp_name], 'r');
	$content = base64_encode(fread($fh, $_FILES[$key][size]));
	fclose($fh);
	return $content;
}

/**
Returns an array of the types of form widgets understood by form.h 
*/
function form_get_types() {
	return array( 'email' , 'text' , 'bigtext', 'richtext', 'label' , 'password' , 'richtext', 'bigtext' , 'integer' , 'dollars' , 'float' , 'date' , 'timestamp' , 'flag' , 'none' , 'menu' , 'file', 'phone' , 'url');
}

/** Automatically generate a form based on what the database says about it.
*/
function form_auto($table_name, $pkey_value) {
	$relid = db_value('SELECT c.oid FROM pg_catalog.pg_class c LEFT JOIN pg_catalog.pg_namespace n ON n.oid = c.relnamespace
	 WHERE c.relname ~ ?  AND pg_catalog.pg_table_is_visible(c.oid)', '^('.$table_name.')$');

	$pkey_column = db_pkey_for_table($table_name);


	$row = db_row("select * from $table_name where $pkey_column = ?", $pkey_value);

	$columns = db_multirow('
	   SELECT a.attname,
	     pg_catalog.format_type(a.atttypid, a.atttypmod),
	       (SELECT substring(pg_catalog.pg_get_expr(d.adbin, d.adrelid) for 128)
	          FROM pg_catalog.pg_attrdef d
		     WHERE d.adrelid = a.attrelid AND d.adnum = a.attnum AND a.atthasdef) as default_val,
		       a.attnotnull, a.attnum
		       FROM pg_catalog.pg_attribute a
		       WHERE a.attrelid = ? AND a.attnum > 0 AND NOT a.attisdropped
		       ORDER BY a.attnum
		  ', $relid);
	$rc = array();
	foreach ($columns as $c) {
		switch ($c['format_type']) {
			case 'integer':
				$rc[$c['attname']] = array('type'=>'integer');
				break;

			case 'text':
				$rc[$c['attname']] = array('type'=>'text');
				break;

			case 'smallint':
				$rc[$c['attname']] = array('type'=>'flag');
				break;

			case 'timestamp without time zone':
				$rc[$c['attname']] = array('type'=>'timestamp');
				break;

			default:
		}
		$rc[ $c['attname'] ]['default'] = $row[ $c['attname'] ];
		if ($c['attnotnull']=='t') {
			$rc[ $c['attname'] ]['required'] = 1;
		}


	}

	return $rc;
}

/** Handle a form populated with the above.
*/
function form_auto_handle($table_name, $pkey_value) {
	$pkey_column = db_pkey_for_table($table_name);
	db_update($table_name, $pkey_column, $pkey_value, $_POST);
}

/* internal functions follow */
function form__error_for_widget($name) {
	global $_form_errors;
	if ($err = $_form_errors[$name]) {
        if ($fn = config_get('form', 'custom_error_formatter')) {
            return $fn($err);
        } 

		return '<span class="error"> '.$err.' </span>';
	}
	return '';
}

function form__menu2html($menu, $cur) {
	$rc = '';
	foreach ($menu as $k=>$v) {
		if (is_array($v)) {
			$rc .= "<optgroup label=\"${v['label']}\">". form__menu2html($v['menu'], $cur).'</optgroup>';
		} else {
			$sel = (("$cur"==="$k") || (is_array($cur)&&in_array($k, $cur))) ? ' selected ' : '';
			$rc .= '<option value="'.$k."\" $sel >".$v."</option>\n";
		}
	}
	return $rc;
}

function form__html_for_widget($name, &$widget, $errors=array()) {
    global $_form_name;
	$type = null;
	$constraints = array();

	$type = $widget['type'];
	$constraints = &$widget;

	# deal with form errors	
	if ($errors[$name]) {
		$error = '<span class="error"> '.$errors[$name].' </span>';
	}

	# in some circumstances, we are rendering a fraction of a widget
	# so look for widget.subkey
	if ($constraints['customlayout']) {
		$pos = strpos($name,'.');
		$k = substr($name, $pos+1);
		$name = substr($name, 0, $pos);	
	}

	# render the form: first, determine with that current value of the widget is
	if ($_REQUEST[$name] != '') {
          $cur = $_REQUEST[$name];
	} else {
	      $cur = $constraints['default'];
	}
    if ($_REQUEST[$name."_file_cur"]=='delete') {
		$cur = '';
	}

	# value is deprecated; use default instead
	if (isset($constraints['value'])) {
        $cur = $constraints['value'];
	}

    # some widgets get special defaults
    if ($type=='url' && $cur=='') {
      $cur='http://';
    }

	$orig_cur = $cur;
	if (!is_array($cur)) { $cur = htmlspecialchars($cur); }

	$extra = $constraints['extra'];
	$autoid = $_form_name.'_'.$name;
	$autoid = str_replace('[','_', str_replace(']','', $autoid));

	if (isset($constraints['size'])) {
		$extra .= ' size="'.$constraints['size'].'" ';
	}
	if (isset($constraints['rows'])) {
		$extra .= ' rows="'.$constraints['rows'].'" ';
	}
	if (isset($constraints['cols'])) {
		$extra .= ' cols="'.$constraints['cols'].'" ';
	}
	if (isset($constraints['maxlength'])) {
		$extra .= ' maxlength="'.$constraints['max'].'" ';
	}

	if ($error) {
		$extra .= ' class="'.$type.' '.$_form_name.'_'.$name.' error" ';
	} else {
		$extra .= ' class="'.$type.' '.$_form_name.'_'.$name.'" ';
	}
	
	if ($constraints['tabindex']) { $extra .= ' tabindex="'.$constraints['tabindex'].'" '; }	
	else { $extra .= ' tabindex="100" '; }

    if ($constraints['autocomplete_sql']) {
        $constraints['autocomplete'] = db_array($constraints['autocomplete_sql'], $constraints['autocomplete_sql_params']);
    }	
    if ($constraints['autocomplete']) {
        $autocomplete = "<div id=\"{$autoid}_autocomplete\" class=\"auto_complete\"></div><script type=\"text/javascript\">new Autocompleter.Local('${autoid}', '{$autoid}_autocomplete', ['".join("','", $constraints['autocomplete'])."'], { partialSearch:true,fullSearch:true } ); </script>";
        $autocomplete = "<script type=\"text/javascript\">$(\"#{$autoid}\").autocomplete( ['".join("','", $constraints['autocomplete'])."'] ); </script>";
    }

    switch ($type) {
	case 'urifragment':
		if (!isset($constraints['size'])) {
			$extra .= ' size="20"';
		}
		return "<input id=\"$autoid\" $extra type=text name=\"".$name.'" value="'.$cur.'" />'.$autocomplete.$error;
		break;
		
	case 'email':
		if (!isset($constraints['size'])) {
			$extra .= ' size="30"';
		}
		# fall through to text

	case 'text':
		if (!isset($constraints['size'])) {
			$extra .= ' size="30"';
		}
		return "<input id=\"$autoid\" $extra type=text name=\"".$name.'" value="'.$cur.'" />'.$autocomplete.$error;
		break;

	case 'label':
		return "<a id=\"$autoid\" class=\"formlabel\" name=\"".$name."\">".$cur."</a>";

	case 'password':
		return "<input id=\"$autoid\" $extra type=password name=\"".$name.'" value="'.$cur.'" />'.$error;
		break;
		
	case 'richtext':
            $editor = new CKEditor($name);
            $editor->BasePath = "/.ckeditor/";
            CKFinder::SetupCKEditor($editor, '/.ckfinder/');
            $editor->Config["CustomConfigurationsPath"] = href("/.ckeditor/myconfig.php",'theme', $constraints['theme'])  ;
            $editor->Value = $orig_cur;
            $editor->Width = $constraints['width'] ?  $constraints['width'] : '600';
            $editor->Height = $constraints['height'] ? $constraints['height'] : '400';

            return "<textarea rows=\"20\" cols=\"60\" id=\"$autotid\" $extra name=\"$name\">$orig_cur</textarea><script type=\"text/javascript\">CKEDITOR.basePath = '/.ckeditor/'; CKEDITOR.replace( '$name', { customConfig:'myconfig.php', 'filebrowserBrowseUrl':'/.ckfinder/ckfinder.html' } )</script>";
            break;

	case 'bigtext':
		return "<textarea id=\"$autoid\" $extra name=\"$name\">$cur</textarea>$error";
		break;
		
	case 'mdtext':
		return "<textarea id=\"$autoid\" class=\"cpf_markdown\" $extra name=\"$name\">$cur</textarea>$error";
		break;
		
	case 'dollars':
		$cur = sprintf('%01.2f', $cur);
	case 'integer':
	case 'float':
		if (!isset($constraints['size'])) {
			$extra .= ' size="5"';
		}
		return "<input id=\"$autoid\" $extra type=text name=\"".$name.'" value="'.$cur.'" />'.$error;
		break;

	case 'date':
		return "<input id=\"$autoid\" $extra type=text size=\"12\" maxlength=\"30\" name=\"".$name.'" value="'.$cur.'" />'.$error;
		break;

	case 'time':
		return "<input id=\"$autoid\" $extra type=text size=\"8\" maxlength=\"30\" name=\"".$name.'" value="'.$cur.'">'.$error;
		break;

	case 'timestamp':
		return "<input id=\"$autoid\" $extra type=text size=\"28\" maxlength=\"30\" name=\"".$name.'" value="'.$cur.'" />'.$error;
		break;
	
	case 'flag':
		if ($cur == 0) $cur=0;
		return "<input id=\"$autoid\" $extra type=checkbox name=\"".$name.($cur ? '" CHECKED' : '"').' value="1" />'.$error;
		break;

	case 'none': 
		return $error;
		break;

	case 'document':
		$sql_extra = '';
		if ($widget['category_id']) {
			$sql_extra = ' where document_category_id=?';
		} 
		$documents = db_multirow('select document_id as key from documents '.$sql_extra, $widget['category_id']);
		i18n_get_batch($documents, 'document', 'key');
		foreach ($documents as $d) {
			$widget['menu'][$d['key']] = $d['name'];
		}
		$widget['type'] = 'menu';

		
	case 'menu':
		if ($widget['sql'] && !$widget['menu']) {
			$results = db_multirow($widget['sql'], $widget['sqlp1'], $widget['sqlp2'], $widget['sqlp3'], $widget['sqlp4']);
			foreach ($results as $r) {
				$widget['menu'][$r['key']]=$r['value'];
			}
		}

        if ((sizeof($widget['menu'])<7 || $widget['style'] == 'checkboxes') && $widget['multiple']) {
            # checkboxes it is
            $cols = $widget['cols'];
            if ($cols) {
                $td_width_string = ' width="'.round(100/$cols).'%" ';
            }
            $cur_col=0;
            $cur_idx=0;
    
            $cbgroup = array('0'=>array(), '1'=>array());
            if (sizeof($widget['menu'])>40) {
                foreach ($widget['menu'] as $k=>$v) {
                  if (is_array($cur)) {
                    $checked = in_array($k, $cur) ? 0 : 1;
                  } else {
                    $checked = ($cur==$k) ? 0 : 1;
                  }
                  $cbgroup[$checked][$k]=$v;
                } 
            } else {
                $cbgroup[0] = $widget['menu'];
            }
            foreach ($cbgroup as $checked=>$menu) {
                if (!$menu) continue;
                if ($checked) {
                    $classname = 'checkchoices_'.$name;
                    $txt .= "<a href=\"#\" class=\"shower_$classname\" onclick=\"$('.shower_$classname').hide(1.0); $('.hider_$classname').show(1.0); $('.$classname').show(2); return false\">Show ".sizeof($menu)." choices...</a> 
                             <a href=\"#\" class=\"hider_$classname\" style=\"display:none\" onclick=\"$('.shower_$classname').show(1.0); $('.hider_$classname').hide(1.0); $('.$classname').hide(2); return false\">Hide choices...</a>
                                 <table class=\"$classname\" style=\"display:none\">";
                } else {
                    $txt .= '<table >';
                }
                foreach ($menu as $k=>$v) {
                  $txt .= ($cur_col==0) ? "<tr>" :"";

                  if (is_array($cur)) {
                    $sel = in_array($k, $cur) ? ' checked ' : '';
                  } else {
                    $sel = ($cur==$k) ? ' checked ' : '';
                  }
                  $txt .= "<td valign=\"top\" width=\"1%\"><input id=\"$autoid-$k\" $extra type=\"checkbox\" name=\"".$name."[]\" value=\"$k\" $sel /></td><td $td_width_string valign=\"top\"><label for=\"$autoid-$k\">$v</label> \n";
                
                  if (++$cur_col==$cols) {
                        $txt .= '</td></tr>';
                        $cur_col=0;
                  } else {
                        $txt .= '</td>';
                  }
                }
                $txt .= ($cur_col==0) ? '</table>' : '</tr></table>';
            }
            return $error.$txt;

          } else if ($widget['radio']) {

            $radio_display = $widget['style']=='inline' ? 'inline' : 'block';

            if ($widget['menu']) 
                    if ($widget['customlayout']) {
                        if ($k) {
                                $sel = ($cur==$k) ? ' checked ' : '';
                                $txt .= "<input class=\"radio\" id=\"$autoid\" $extra type=radio name=\"".$name.'" value="'.$k."\" $sel ><label for=\"$autoid\"> $v</label> \n";
                        }
                    } else {
                        if ($widget['style']=='tabular') {
                            $txt .= "\n<table class=\"cpf_radiobuttontable\">";
                            $cur_col = 0;
                            $cols = $widget['cols'];
                            if ($cols) {
                                $td_width_string = ' width="'.round(100/$cols).'%" ';
                            }
                        }


                        foreach ($widget[menu] as $k=>$v) {
                          $sel = ($cur==$k) ? ' checked ' : '';
                          if ($widget['style']=='tabular')  $txt .= ($cur_col==0) ? "<tr><td>" :"<td>";
                      
                          if (!is_array($v)) {
                              $button = "<input class=\"radio\" id=\"${autoid}_$k\" $extra type=radio name=\"".$name.'" value="'.$k."\" $sel />";
                              $label = "<label for=\"${autoid}_$k\">$v</label>";
                          } else { 
                              $button = "<input class=\"radio\" id=\"${autoid}_$k\" ".$v['extra']."$extra type=radio name=\"".$name.'" value="'.$k."\" $sel / >";
                              $label = "<label for=\"${autoid}_$k\">".$v['value']."</label>";
                          }

                          $txt .= "<div class=\"cpf_radiobutton\" style=\"display: $radio_display\">$button$label</div> ";

                          if ($widget['style']=='tabular')  
                              if (++$cur_col==$cols) {
                                    $txt .= '</td></tr>';
                                    $cur_col=0;
                              } else {
                                    $txt .= '</td>';
                              }
                        }
                        if ($widget['style']=='tabular')  
                            $txt .= ($cur_col==0) ? '</table>' : '</tr></table>';
                    }
            return $txt.$error;
          }
		
		if (!$widget['required'] && !$widget['multiple'] && is_array($widget['menu'])) { $widget[menu] = array(''=>'&nbsp;')+ $widget[menu]; if (!$cur) $cur=''; }
		if (!$widget['menu']) { $widget['menu'] = array(); }


		$txt .= form__menu2html($widget['menu'], $cur);

		if ($widget['multiple']) {
            $nameextra = '[]';
            if ($widget['rows']) {
                $extra .= " MULTIPLE SIZE=\"$widget[rows]\" ";
            } else {
                $extra .= ' MULTIPLE ';
            }
		}
		return "<select id=\"$autoid\" $extra name=\"$name$nameextra\">$txt</select> $error";
		break;

	case 'file':
		//$cur is the upload_id
		$result = "";
		if ($cur) {
		   $cur_data = db_row("select * from uploads where upload_id=?",$cur);
		   $result .= "<span id='${name}_file_replace'>"
		       ."<input type=hidden name='${name}_file_cur' value='keep' id='${name}_file_cur'>"
		       ."<input type=hidden name='${name}_file_orig' value='$cur'> "
		       ."<span><a href='/documents/upload.php?upload_id=$cur' target='other'>$cur_data[original_name]&nbsp;-&nbsp;".round($cur_data[size]/1024)."&nbsp;kb</a></span> "
		       ."(<a href='' onclick=\""
			  ."widget = document.getElementById('${name}_file_cur'); "
			  ."widget.value = 'change'; "
			  ."widget = document.getElementById('${name}_file_replace'); "
			  ."widget.style.display = 'none'; "
			  ."widget = document.getElementById('${name}_file_select'); "
			  ."widget.style.display = 'inline'; "
			  ."return false;"
			  ."\">change</a>) "
		       ."(<a href='' onclick=\""
			  ."widget = document.getElementById('${name}_file_cur'); "
			  ."widget.value = 'delete'; "
			  ."widget = document.getElementById('${name}_file_replace'); "
			  ."widget.style.display = 'none'; "
			  ."widget = document.getElementById('${name}_file_select'); "
			  ."widget.style.display = 'inline'; "
			  ."return false;"
			  ."\">delete</a>)"
		     ."</span>\n";
		    $show_selector = "none";
		} else {
		    $show_selector = "inline";
		}
		$result  .= "<span style='display: $show_selector;' id='${name}_file_select'><input type=file size=\"15\" name='$name'></span>\n";
		return "<span class=filewidget id=\"$autoid\" $extra>$result</span>$error";
		break;

	case 'visible':
		return $cur;
		break;

	case 'hidden':
		return "<input id=\"$autoid\" $extra type=hidden name=\"".$name.'" value="'.$cur.'" />';
		break;

	case 'submit':
		if (!$cur) {
			$cur = beautify_identifier($name); 
		}

		if ($cur) {
			$extra .= " value=\"$cur\" ";
		}

		if ($widget[src]) {
			return "<input id=\"$autoid\" $extra type=image name=\"$name\" src=\"".$widget[src].'">';
		} else {
			return "<input id=\"$autoid\" $extra type=submit name=\"$name\" />"; 
		}

		break;

	case 'phone':
			if (!isset($constraints['size'])) {
				$extra .= ' size="15"';
			}
			return "<input id=\"$autoid\" $extra type=text name=\"".$name.'" value="'.$cur.'" /> <!-- fixme this should be better -->'.$autocomplete.$error;
			break;

	case 'url':
			if (!isset($constraints['size'])) {
				$extra .= ' size="45"';
			}
			return "<input id=\"$autoid\" $extra type=text name=\"".$name.'" value="'.$cur.'" /> '.$autocomplete.$error;
			break;

    case 'captcha':
        require_once 'recaptchalib.php';
        return recaptcha_get_html($constraints['public-key']);
        break;

	default:
		return "Unrecognized widget type for $name";
	}
}

# historical, deprecated in favour beautify_identifier
function form_beautify_identifier($id) {
  return beautify_identifier($id);
}

function _form_handle_submission($name) {
    $globalize_list = array();

	if (!(form_was_submitted() === $name)) { return false; }
	unset ($_POST['_form_id']);

	global $_form_contents,$_form_errors;
	$form = &$_form_contents[$name];
	while (list($key, $widget) = each($form)) {
		$val = &$_POST[$key];

		# clean up input a bit

		# map a menu with nothing chosen to NULL
		if ($widget['type']=='menu') {
		    if ($val === '') {
			$val = null;
			unset($_REQUEST[$key]);
			unset($_POST[$key]);
		    }
		}

		# trim down text, bigtext, and richtext, url, various others
		if ($widget['type'] == 'text' || $widget['type']=='url' || $widget['type'] == 'urifragment' || $widget['type'] == 'email' || $widget['type'] == 'richtext' || $widget['type'] == 'bigtext') {
			$val = trim($val);
		}

		if ($widget['type'] == 'flag' && !$val) {
			$val = '0';
		}

		if ($widget['type'] == 'visible') {
			$val = $widget['value'];
		}

        if ($widget['type'] == 'url') {
            if ($val == 'http://' || $val=='') {
                $val = '';
            } else if (substr($val, 0, 7) != 'http://' && substr($val, 0, 8) != 'https://' && $widget['http-only']) {
                $val = "http://$val";
            }
            if ($val && !is_url($val)) {
                $_form_errors[$key] = "This doesn't look like a URL";
            }
        }

        if ($widget['type'] == 'captcha') {
            require_once 'recaptchalib.php';
            $resp = recaptcha_check_answer ($widget['private-key'],
                                $_SERVER["REMOTE_ADDR"],
                                $_POST["recaptcha_challenge_field"],
                                $_POST["recaptcha_response_field"]);
            if (!$resp->is_valid) {
                if ($resp->error == 'recaptcha-not-reachable') {
                    // this is ok
                } else if ($resp->error == 'incorrect-captcha-sol') {
                    $_form_errors[$key] = _('Please enter the words that are shown.');
                }  else {
                    $_form_errors[$key] = 'Strange error from re:captcha: '.$resp->error;
                }
            }

        }

        # don't dick with this rule casually
		if (!$widget['required'] && $widget['type'] != 'file' && $val == '') {
			$globalize_list[$key] = null;
			continue;
		}

        # don't dick with this rule casually
		if ($widget['required'] && (is_null($val) || $val === '') && !($widget['type'] == 'file' && $_FILES[$key]['tmp_name'] )) {
			$_form_errors[$key] = _('This field is required.');
		}

		# handle min/max and numeric widgets {
                $is_numeric_widget = ($widget['type'] == 'integer' || $widget['type'] == 'float');

                if (isset($widget['min'])) {
                    if ($is_numeric_widget  &&  $val <= $widget['min']) {
                        $_form_errors[$key] = _('This field must be bigger than ').$widget['min'];
                    }
                    if (!$is_numeric_widget &&  strlen($val) < $widget['min']) {
                        $_form_errors[$key] = _('This field must be at least ').$widget['min'].' letters long.';
                    }
                }
                if (isset($widget['max'])) {
                    if ($is_numeric_widget  &&  $val >= $widget['max']) {
                        $_form_errors[$key] = _('This field must be smaller than ').$widget['max'];
                    }
                    if (!$is_numeric_widget &&  strlen($val) > $widget['max']) {
                        $_form_errors[$key] = sprintf(_('This field can be at most %s letters long.'), $widget['max']);
                    }
                }

                if ($widget['type'] == 'integer' && !is_integer(0+$val)) {
                    $_form_errors[$key] = _("This field can only have whole numbers.");
                }
                if ($widget['positive'] && $val <= 0) {
                    $_form_errors[$key] = _("This field must be positive.");
                }
                if ($is_numeric_widget) {
                    $val = str_replace(',', '', $val);
                    if (!is_numeric($val)) {
                        $_form_errors[$key] = _("This field must be numeric.");
                    }
                }
		# } done with min/max/numeric

		if ($widget['type']=='urifragment') {
			if (!preg_match('/^[a-zA-Z\-\_0-9]+$/', $val)) {
					$_form_errors[$key] = _('Alphanumeric with some punctuation only.');
			}
		}
				
		if ($widget['type']=='dollars') {
			preg_match('/\$?([0-9\-\.\,\+]*)/', $val, $matches);
			$val = $matches[1];
			$val = str_replace(',','',$val);
			$val = sprintf("%01.2f", $val);
		}

		if ($widget['type']=='date') {
			if (preg_match('/(\d{4}).(\d\d?).(\d\d?)/', $val, $matches)) {
				$val = $matches[1].'-'.$matches[2].'-'.$matches[3];
			}

			if (strlen($val) > 0 && ($result=strtotime($val)) !== -1) {
				$val = date('Y-m-d', $result);
			} elseif(strlen($val) > 0) {
				$_form_errors[$key] = _("This field must be of the form yyyy-mm-dd.");
			}
		}

		if ($widget['type']=='timestamp') {
			if (substr($val, 19, 1)=='.') {
				$val = substr($val,0,19);
			}
			if (($result=strtotime($val)) !== -1) {
				$val = date('Y-m-d G:i:s', $result);
			} else {
				$_form_errors[$key] = _("This field must be of the form yyyy-mm-dd h:m:s.");
			}
		}

		if ($widget['type'] == 'email') {
			$val = strtolower($val);
			if (!cpf_is_email($val)) {
				$_form_errors[$key] = _('This field must be an e-mail address');
			}
		}

		if ($widget['type'] == 'file' && $_FILES[$key]['type']  ) {
            $file_details = $_FILES[$key];

            if ($widget['file_min_size']) {
                if ($file_details['size']<$widget['file_min_size']) {
                    $sz = $widget['file_min_size'];
                    if ($sz > 1024 * 1024) {
                        $sz = sprintf("%d MB", $sz/1024/1024);
                    } else {
                        $sz = sprintf("%D KB", $sz/1024);
                    }
                    form_error($key, "File should be at least $sz.");
                }
            }
            if (is_array($widget['file_require_mime'])) {
                $matched = 0;
                foreach ($widget['file_require_mime'] as $regex) {
                    if (preg_match('#'.$regex.'#', $file_details['type'])) $matched++;
                }
                if (!$matched) {
                    if  ($widget['file_require_mime_pretty']) {
                        form_error($key, $widget['file_require_mime_pretty']);                        
                    } else {
                        form_error($key, 'This file is not one of the accepted types.');
                    }
                }
            }
            
            if ($widget['file_image_aspect'] || $widget['file_resolution']) {
                 list($x, $y, $ignored, $ignored) = getimagesize($file_details['tmp_name']);
                 $stop_bitching=0;
                 
                 if ($widget['file_image_aspect']) {
                    list ($amin, $amax) = split(',', $widget['file_image_aspect']);
                    $ratio = $x/$y;
                    
                    if (($amin && $ratio<$amin) || ($amax && $ratio>$amax)) {
                        $amin += 0;
                        $amax += 0;
                        form_error($key, "This image's aspect ratio must be $amin-$amax.");
                        $stop_bitching++;
                    }
                }
                 
                 if (!form__error_for_widget($key) && $widget['file_resolution']) {
                     list ($xrange, $yrange) = split('x', $widget['file_resolution']);
                     list ($xmin, $ymax) = split(',', $xrange);
                     list ($ymin, $ymax) = split(',', $yrange);
        
                     if (($xmin && $x<$xmin) || ($xmax && $x>$xmax)) {
                            if ($xmax && $xmin) {
                                  form_error($key, "This image must $xmin-$ymax pixels wide.");
                              } else if (!$xmax) { 
                                  form_error($key, "This image must be at least $xmin pixels wide."); 
                              } else if (!$xmin) { 
                                  form_error($key, "This image must be no more than $xmax pixels wide."); 
                              }
                    }
                    
                    if (($ymin && $y<$ymin) || ($ymax && $y>$ymax)) {
                         if ($ymax && $ymin) {
                             form_error($key, "This image must $ymin-$ymax pixels tall: got $y");
                         } else if (!$ymax) { 
                             form_error($key, "This image must be at least $ymin pixels tall."); 
                         } else if (!$ymin) { 
                             form_error($key, "This image must be at least $ymax pixels tall."); 
                         }
                    }
                 }
                 
                 if ($widget['file_image_alpha']) {
                    if ($file_details['type'] != 'image/png') { form_error("Must be an PNG image."); } else {
                        $im = imagecreatefrompng($file_details['tmp_name']);
                        $mode = $widget['file_image_alpha'];
                        $width = imagesx($im);
                        $height = imagesy($im);

                        $foundtrans=0;
                        for ($x=0; $x<$width; $x++) {
                            for ($y=0; $y<$width; $y++) {
                                $alpha = imagecolorat($im, $x, $y) >> 24;
                                if (isset($alpha)) {
                                    if ($alpha==127 && $mode=='allopaque') {
                                        form_error($key, "This image must not have a clipping path.");
                                        break 2;
                                    }
                                    if ($alpha==127 && $mode=='sometransparent') {
                                        $foundtrans=1;
                                        break 2;
                                    }
                                }
                            }
                        }
                        if ($mode=='sometransprent' && !$foundtrans) {
                            form_error($key, "This image must have a clipping path.");
                        }
                    }
                 }
                 
                 
                if (!form__error_for_widget($key) && $widget['file_final_resolution']) {
                     system("mogrify -geometry ".$widget['file_final_resolution'].' '. $file_details['tmp_name']);
                }
                if (!form__error_for_widget($key) && $widget['file_final_format']) {
                    system("mogrify -format ".$widget['file_final_format'].' '. $file_details['tmp_name']);
                }
            }
        }

		if ($widget['type'] != 'file') {
			$globalize_list[$key] = $val;
			$_POST[$key] = $_REQUEST[$key] = $val;
		}
	}
 
	if (form_valid($name)) {
		# mop up file uploads and log them
		reset($form);
		while (list($key, $widget) = each($form)) {
			  if ($widget['type']=='file' && $_FILES[$key]['tmp_name']) {
                # see if we can yank some data out of the file
			  	$val = $_FILES[$key]['size'];

                $image_details = getimagesize($_FILES[$key]['tmp_name']);
                if (!is_array($image_details)) { $image_details = array(); }

				$upload_dest = config_get('files','upload_directory');
				if (!$upload_dest) {
					$upload_dest = '../uploads/';
				}
				$upload_dest .= getmicrotime();
				$filesize = filesize($_FILES[$key]['tmp_name']);
				chmod($_FILES[$key]['tmp_name'], 0770);
				$result = move_uploaded_file($_FILES[$key]['tmp_name'], $_SERVER['DOCUMENT_ROOT'].'/'.$upload_dest);
				if ($result) {
					$upload_id = db_newid('upload_id');
					db_send('insert into uploads(upload_id, original_name, mime_type, size, local_filename, image_width, image_height) values (?,?,?,?,?,?,?)',
                        $upload_id,
                        $_FILES[$key]['name'],
                        $_FILES[$key]['type'],
                        $filesize, 
                        $upload_dest, 
                        $image_details[0],
                        $image_details[1]);

					$_FILES[$key]['tmp_name'] = $upload_dest;
					$_FILES[$key]['upload_id'] = $upload_id;

                    //temporary fix to support generic form handling in i18n
                    //  see ticket#11504
                    $_REQUEST[$key] = $upload_id;

					global ${$key};
					global ${$key.'_upload_id'};
					global ${$key.'_size'};
					global ${$key.'_name'};
					global ${$key.'_mimetype'};

					${$key} = $_FILES[$key]['tmp_name'];
					${$key.'_upload_id'} = $_FILES[$key]['upload_id'];
					${$key.'_size'} = $_FILES[$key]['size'];
					${$key.'_name'} = $_FILES[$key]['name'];
					${$key.'_mimetype'} = $_FILES[$key]['type'];
			      }
			}
		}
		foreach ($globalize_list as $k=>$v) {
			global ${$k};
			$$k= $v;
		}
	}
}

?>
