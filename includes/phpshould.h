<?php

# Returns the current time in microseconds.
function getmicrotime() {
	$mtime = microtime(); 
	$mtime = explode(' ',$mtime); 
	$mtime = $mtime[1] + $mtime[0]; 
	return $mtime;
}

if ($_SERVER['HTTP_X_FORWARDED_FOR']) { $_SERVER['REMOTE_ADDR'] = $_SERVER['HTTP_X_FORWARDED_FOR']; }

function is_httpunit() {
    return (substr($_SERVER["HTTP_USER_AGENT"], 0, 8) == 'httpunit');
}

function read_whole_file($file) {
	$fd = fopen ($file, 'r');
	$contents = fread ($fd, filesize ($file));
	fclose ($fd);
	return $contents;
}

function redirect($where) {
	session_write_close();
	if (substr($where, 0, 4)=='http') {
		header("Location: $where");
	} else if (substr($where, 0, 1)=='/') {
		if ($_SERVER['HTTPS']=='on') {
			header("Location: https://".$_SERVER['HTTP_HOST'].$where);
		} else {
			header("Location: http://".$_SERVER['HTTP_HOST'].$where);
		}
	} else {
		header("Location: $where");
	}
	flush();
	die;
}

function a_href() {
	$arg_list = func_get_args();

	return '<a href="'.  call_user_func_array('href',$arg_list).'" ';
}

function href() {
	$numargs = func_num_args()-1;
	$arg_list = func_get_args();
	$base= array_shift($arg_list);
	if (strpos($base,'?')) { $already_q = 1; }
	if (is_array($arg_list[0])) {
		$i=0;
		foreach ($arg_list[0] as $k=>$v) {
			if (is_null($v)) {
				$v = "\N";
			}
			$base .= ($i++==0 && !$already_q ? '?' : '&') . $k.'='.urlencode($v);
		}
	} else {
		for ($i=0; $i<$numargs; $i+=2) {
			if (is_null($arg_list[$i+1])) {
				$arg_list[$i+1] = "\N";
			}
			$base .= ($i==0 && !$already_q ? '?' : '&') . $arg_list[$i].'='.urlencode($arg_list[$i+1]);
		}
	}
	
	return $base;
}

function config_get($realm, $var) {
    global $Gconfig;
    return $Gconfig[$realm][$var];
}

function pre($sql_string) {
    print '<pre>'.$sql_string.'</pre>';
}


/************************************************
** Title.........: Debug Lib
** Version.......: 0.5.3
** Author........: Thomas Schüßler <tulpe@atomar.de>
** Filename......: debuglib.php(s)
** Last changed..: 24. February 2003
** License.......: Free to use. Postcardware ;)
**
*************************************************
**
** Functions in this library:
**
** print_a( array array [,int mode] )
**   prints arrays in a readable, understandable form.
**   if mode is defined the function returns the output instead of
**   printing it to the browser
**   
**   
** show_vars([int mode])
**   use this function on the bottom of your script to see all
**   superglobals and global variables in your script in a nice
**   formated way
**   
**   show_vars() without parameter shows $_GET, $_POST, $_SESSION,
**   $_FILES and all global variables you've defined in your script
**
**   show_vars(1) shows $_SERVER and $_ENV in addition
**
** History: (starting with version 0.5.3 at 2003-02-24)
**
**   - added tooltips to the td's showing the type of keys and values (thanks Itomic)
**   
************************************************/


# This file must be the first include on your page.

/* used for tracking of generation-time */
{
    $MICROTIME_START = microtime();
    @$GLOBALS_initial_count = count($GLOBALS);
}
    
/************************************************
** print_a class and helper function
** prints out an array in a more readable way
** than print_r()
**
** based on the print_a() function from
** Stephan Pirson (Saibot)
************************************************/

class Print_a_class {
    
    # this can be changed to FALSE if you don't like the fancy string formatting
    var $look_for_leading_tabs = TRUE;

    var $output;
    var $iterations;
    var $key_bg_color = '1E32C8';
    var $value_bg_color = 'DDDDEE';
    var $fontsize = '8pt';
    var $keyalign = 'center';
    var $fontfamily = 'Verdana';
    var $export_flag;
    var $show_object_vars;
    var $export_dumper_path = 'http://tools.www.mdc.xmc.de/print_a_dumper/print_a_dumper.php';
    # i'm still working on the dumper! don't use it now
    # put the next line into the print_a_dumper.php file (optional)
    # print htmlspecialchars( stripslashes ( $_POST['array'] ) );  
    var $export_hash;
        
    function Print_a_class() {
        $this->export_hash = uniqid('');
    }
    
    # recursive function!
    function print_a($array, $iteration = FALSE, $key_bg_color = FALSE) {
        $key_bg_color or $key_bg_color = $this->key_bg_color;
        
            # if print_a() was called with a fourth parameter (1 or 2)
            # and you click on the table a window opens with only the output of print_a() in it
            # 1 = serialized array
            # 2 = normal print_a() display
            
            /* put the following code on the page defined with $export_dumper_path;
            --->%---- snip --->%----
            
                if($_GET['mode'] == 1) {
                    print htmlspecialchars( stripslashes ( $_POST['array'] ) );
                } elseif($_GET['mode'] == 2) {
                    print_a(unserialize( stripslashes($_POST['array'])) );
                }

            ---%<---- snip ---%<----
            */
            
        if( !$iteration && $this->export_flag ) {
            $this->output .= '<form id="pa_form_'.$this->export_hash.'" action="'.$this->export_dumper_path.'?mode='.$this->export_flag.'" method="post" target="_blank"><input name="array" type="hidden" value="'.htmlspecialchars( serialize( $array ) ).'"></form>';
        }
        
        # lighten up the background color for the key td's =)
        if( $iteration ) {
            for($i=0; $i<6; $i+=2) {
                $c = substr( $key_bg_color, $i, 2 );
                $c = hexdec( $c );
                ( $c += 15 ) > 255 and $c = 255;
                $tmp_key_bg_color .= sprintf( "%02X", $c );
            }
            $key_bg_color = $tmp_key_bg_color;
        }
        
        # build a single table ... may be nested
        $this->output .= '<table style="border:none;" cellspacing="1" '.( !$iteration && $this->export_flag ? 'onClick="document.getElementById(\'pa_form_'.$this->export_hash.'\').submit();" )' : '' ).'>';
        foreach( $array as $key => $value ) {
            
            $value_style = 'color:black;';
            $key_style = 'color:white;';
            
            $type = gettype( $value );
            # print $type.'<br />';
            
            # change the color and format of the value
            switch( $type ) {
                case 'array':
                    break;
                
                case 'object':
                    $key_style = 'color:#FF9B2F;';
                    break;
                
                case 'integer':
                    $value_style = 'color:green;';
                    break;
                
		case 'NULL':
		    $value_style = "color: grey";
		    $value = '(null)';
		    break;

                case 'double':
                    $value_style = 'color:red;';
                    break;
                
                case 'bool':
                    $value_style = 'color:blue;';
                    break;
                    
                case 'resource':
                    $value_style = 'color:darkblue;';
                    break;
                
                case 'string':
                    if( $this->look_for_leading_tabs && preg_match('/^\t/m', $value) ) {
                        $search = array('/\t/', "/\n/");
                        $replace = array('&nbsp;&nbsp;&nbsp;','<br />');
                        $value = preg_replace( $search, $replace, htmlspecialchars( $value ) );
                        $value_style = 'color:black;border:1px gray dotted;';
                    } else {
                        $value_style = 'color:black;';
                        $value = nl2br( htmlspecialchars( $value ) );
                    }
                    break;
            }

            $this->output .= '<tr>';
            $this->output .= '<td nowrap align="'.$this->keyalign.'" style="background-color:#'.$key_bg_color.';'.$key_style.';font:bold '.$this->fontsize.' '.$this->fontfamily.';" title="'.gettype( $key ).'['.$type.']">';
            $this->output .= $key;
            $this->output .= '</td>';
            $this->output .= '<td nowrap="nowrap" style="background-color:#'.$this->value_bg_color.';font: '.$this->fontsize.' '.$this->fontfamily.'; color:black;">';

            
            # value output
            if($type == 'array') {
                $this->print_a( $value, TRUE, $key_bg_color );
            } elseif($type == 'object') {
                if( $this->show_object_vars ) {
                    $this->print_a( get_object_vars( $value ), TRUE, $key_bg_color );
                } else {
                    $this->output .= '<div style="'.$value_style.'">OBJECT</div>';
                }
            } else {
                $this->output .= '<div style="'.$value_style.'" title="'.$type.'">'.$value.'</div>';
            }
            
            $this->output .= '</td>';
            $this->output .= '</tr>';
        }
        $this->output .= '</table>';
    }
}

# helper function.. calls print_a() inside the print_a_class
function print_a( $array, $return_mode = FALSE, $show_object_vars = FALSE, $export_flag = FALSE ) {
    
    if( is_array( $array ) or is_object( $array ) ) {
        $pa = new Print_a_class;
        $show_object_vars and $pa->show_object_vars = TRUE;
        $export_flag and $pa->export_flag = $export_flag;
        
        $pa->print_a( $array );
        
        # $output = $pa->output; unset($pa);
        $output = &$pa->output;
    } else {
        $output = '<span style="color:red;font-size:small;">print_a( '.gettype( $array ).' )</span>';
    }
    
    if($return_mode) {
        return $output;
    } else {
        print $output;
        return TRUE;
    }
}

function script_globals() {
    global $GLOBALS_initial_count;

    $varcount = 0;

    foreach($GLOBALS as $GLOBALS_current_key => $GLOBALS_current_value) {
        if(++$varcount > $GLOBALS_initial_count) {
            /* die wollen wir nicht! */
            if ($GLOBALS_current_key != 'HTTP_SESSION_VARS' && $GLOBALS_current_key != '_SESSION') {
                $script_GLOBALS[$GLOBALS_current_key] = $GLOBALS_current_value;
            }
        }
    }
    
    unset($script_GLOBALS['GLOBALS_initial_count']);
    return $script_GLOBALS;
}

function show_vars($show_all_vars = FALSE, $show_object_vars = FALSE) {
    # Hi Wolfram!!! :))
    $MICROTIME_END        = microtime();
    $MICROTIME_START    = explode(' ', $GLOBALS['MICROTIME_START']);
    $MICROTIME_END        = explode(' ', $MICROTIME_END);
    $GENERATIONSEC        = $MICROTIME_END[1] - $MICROTIME_START[1];
    $GENERATIONMSEC    = $MICROTIME_END[0] - $MICROTIME_START[0];
    $GENERATIONTIME    = substr($GENERATIONSEC + $GENERATIONMSEC, 0, 8);

    if($GLOBALS['no_vars']) return;
    
    # Script zur Anzeige der GET/POST/SESSION/COOKIE/etc.. Variablen
    # einfach am Ende einer PHP Seite includen.
    
    $script_globals = script_globals();
    print '
        <style type="text/css">
        .vars-container {
            font-family: Verdana, Arial, Helvetica, Geneva, Swiss, SunSans-Regular, sans-serif;
            font-size: 8pt;
            padding:5px;
        }
        .varsname {
            font-weight:bold;
        }
        </style>
    ';

    print '<br />
        <div style="border-style:dotted;border-width:1px;padding:2px;font-family:Verdana;font-size:10pt;font-weight:bold;">
        DEBUG <span style="color:red;font-weight:normal;font-size:9px;">(runtime: '.$GENERATIONTIME.' sec)</span>
    ';

    $vars_arr['script_globals'] = array('global script variables', '#7ACCC8');
    $vars_arr['_GET'] = array('GET', '#7DA7D9');
    $vars_arr['_POST'] = array('POST', '#F49AC1');
    $vars_arr['_FILES'] = array('POST FILES', '#82CA9C');
    $vars_arr['_SESSION'] = array('SESSION', '#FCDB26');
    $vars_arr['_COOKIE'] = array('COOKIE', '#A67C52');

    if($show_all_vars) {
        $vars_arr['_SERVER'] =  array('SERVER', '#A186BE');
        $vars_arr['_ENV'] =  array('ENV', '#7ACCC8');
    }

    foreach($vars_arr as $vars_name => $vars_data) {
        if($vars_name != 'script_globals') global $$vars_name;
        if($$vars_name) {
            print '<div class="vars-container" style="background-color:'.$vars_data[1].';"><span class="varsname">'.$vars_data[0].'</span><br />';
            print_a($$vars_name, FALSE, $show_object_vars, FALSE );
            print '</div>';
        }
    }
    print '</div>';
}


function showvar($var, $string = '$var')
{
    $out = "";
    // TRUE, FALSE, NULL and empty arrays: return this value
    if($var === true) {
        $out .= htmlentities($string . ' = TRUE;') . "\n";
    } else if($var === false) {
       $out .= htmlentities($string . ' = FALSE;') . "\n";
    } else if($var === null) {
        $out .= htmlentities($string . ' = NULL;') . "\n";
    } else if($var === array()) {
       $out .= htmlentities($string . ' = array();') . "\n";
    // array or object - foreach element of $var
    } else if ( is_array( $var ) || is_object($var) ) {
        foreach($var as $k=>$v) {
          // Format the string which stands next to the ' = '.
           // [] for arrays and -> for objects
           if(is_array($var)) {
                if(is_string($k)) {
                  $k = "'" . str_replace('\"', '"',
                         addslashes($k)) . "'";
                }
               $new_string = $string . '[' . $k . ']';
            } else if(is_object($var)) {
                $new_string = $string . '->' . $k;
            }
            // dive
            $out .= showvar($v, $new_string);
        }
    // not object, not array
    } else {
        // Format as a string if it is one
      if(is_string($var)) {
            $var = "'" . str_replace('\"', '"',
                     addslashes($var)) . "'";
        }
        $out .= htmlentities($string . ' = ' . $var) . ";\n";
    }
    return $out;
}


function cpf_mail_html($to, $subject, $message, $headers='', $parameters='') {
  if (!$headers) {
       $headers = array();
  } elseif (!is_array($headers)) {
       $headers = preg_split("/\r\n/m", $headers);
  }

  $boundary = uniqid("tinyplanet_cpf_"); 
  $headers[] = "MIME-Version: 1.0";
  $headers[] = "Content-type: multipart/alternative; boundary=\"$boundary\"";

  $message = "--$boundary\nContent-type: text/html; charset=\"UTF-8\"\nContent-Transfer-Encoding: base64\n\n".chunk_split(base64_encode($message)); 
  cpf_mail($to, $subject, $message, $headers, $parameters);
}

//same as below, except $attachments is a list of upload_ids
function cpf_mail_attachment_id($to, $subject, $message, $attachments, $headers='', $parameters='') {
  
  $result = db_multirow("select '".$_SERVER['DOCUMENT_ROOT'].'/'."'||local_filename as filename, original_name as pretty_name, mime_type as content_type "
    ."from uploads where upload_id in (".implode(',',$attachments).")");
  return cpf_mail_attachment($to, $subject, $message, $result, $headers, $parameters);
}

 //$attachments is {{filename, pretty_name, disposition, content_type},...}
function cpf_mail_attachment($to, $subject, $message, $attachments, $headers='', $parameters='') {
	if (!$headers) { 
		$headers = array();
	} elseif (!is_array($headers)) {
		$headers = preg_split("/\r\n/m", $headers);
	}

	$boundary = uniqid("tinyplanet_cpf_"); 
	$headers[] = "MIME-Version: 1.0";
	$headers[] = "Content-type: multipart/mixed; boundary=\"$boundary\"";

	$message = "This is a multi-part message in MIME format.\n\n" .
		"--{$boundary}\n" .
		"Content-type: text/plain; charset=UTF-8\n" .
		"Content-Transfer-Encoding: 8bit\n\n" .
		$message . "\n\n";

	foreach ($attachments as $a) {
		if (!is_readable($a['filename'])) {
			trigger_user_error($a['filename']." doesn't exist or is unreadable", E_USER_ERROR);
			continue;
		}
		$file_data = read_whole_file($a['filename']);
		if ($a['pretty_name']) {
			$pretty_name = $a['pretty_name']; 
		} else {
			$pretty_name = basename($a['filename']);
	  }		

		$disposition = $a['disposition'];
		if (!$disposition) { $disposition = "attachment; filename=\"{$pretty_name}\""; }

		$message .= "--{$boundary}\nContent-type:{$a['content_type']}\nContent-Transfer-Encoding: base64\nContent-Disposition: $disposition\n\n".chunk_split(base64_encode($file_data));
	}
	cpf_mail($to, $subject, $message, $headers, $parameters);
}

function cpf_mail($to, $subject, $message, $headers='', $parameters='') {
	global $Gconfig;
  if (!$headers) {
       $headers = array();
  } elseif (!is_array($headers)) {
       $headers = preg_split("/\r\n/m", $headers);
  }
  if ($Gconfig['site']['owner-email']) {
    $headers[] = 'From: '.$Gconfig['site']['owner'].' <'.$Gconfig['site']['owner-email'].">";
		if (!$parameters) 
			$parameters = " -f ".$Gconfig['site']['owner-email'];
  }
	foreach ($headers as $h) {
		if (strpos($h, "Content-type")!==false) {
			$has_content_type = true;
		}
	}
	if (!$has_content_type) {
		$headers[] = "Content-type: text/plain; charset=\"UTF-8\"";
	}

  $headers_out = join("\n", $headers);

	if ($_SERVER['DEVEL']>0 || is_httpunit() || config_get('email','sandbox_anyway') ) {
		db_send('begin');
		db_send('insert into email_sandbox(email_sandbox_id, to_addr,subject,extra_headers,message,id) values (?,?,?,?,?,?)', db_newid('email_sandbox'), $to, $subject, $headers_out, $message, $headers['cpf_email_id'] );
		db_send('commit');
		$rc = 1;
	} 
	if (!($_SERVER['DEVEL']>0 || is_httpunit()) || config_get('email','sandbox_anyway') ) {
		mb_language('Neutral');
		mb_internal_encoding("UTF-8");
		mb_http_input("UTF-8");
		mb_http_output("UTF-8");
		$rc = mail($to,mb_encode_mimeheader($subject),$message,$headers_out,$parameters);
	}
	return $rc;
}

function img($uri, $extra='') {
    if (substr($uri, 0, 1)!='/') {
        $uri = dirname($_SERVER['PHP_SELF']).'/'.$uri;
    }
    $filename = $_SERVER['DOCUMENT_ROOT'].$uri;
    if (file_exists($filename)) {
        $details = @getimagesize($filename);
        if ($details[0]) {
                $dimensions = $details[3];
        }
        echo "<img $dimensions src=\"$uri\" $extra >";
    }
}

function cpf_is_email($str)  {
		return preg_match('/^[\w\--\_\+]*@[a-zA-Z0-9][\w\.-]*[a-zA-Z0-9]?\.[a-zA-Z][a-zA-Z\.]*[a-zA-Z]$/', $str);
}

// this function makes php4 behave like php5 strtotime
// bugs.php.net/bug.php?id=28717
function strtotime_php5($str) {
	if (($index = strpos($str, '.'))===false) 
		return strtotime($str);
	else 
		return strtotime(substr($str, 0, $index-1));
}

function time_since($timestamp) {
	$diff = time() - $timestamp;

	if ($diff<60) 
			$answer= "$diff second";

	$diff = round($diff/60);
	if (!($answer) && $diff<90) 
			$answer= "$diff minute";

	$diff = round($diff/60);
	if (!($answer) && $diff <48) 
			$answer= "$diff hour";

	$diff = round($diff/24);
	if (!($answer) && $diff < 10) {
			$answer  = "$diff day";
	}

	$diff = round($diff/7);
	if (!($answer)) {
		$answer = "$diff week";
	}

	if ($answer>1) {
			return "${answer}s ago";
	} else {
		return "$answer ago";
	}
}

function beautify_identifier($id) {
  if (substr($id, strlen($id)-3, 3)== '_id') {
	$id = substr($id, 0, strlen($id)-3);
  }
  return ucfirst(str_replace(array('_','-'), array(' ',' '),$id));
}

function humanize_timestamp($ts, $granularity=1) {
    if (!$ts) {  return; } 
    if ($ts<10000) { $ts = strtotime(substr($ts, 0, 19)); }
    $deltaT = time() - $ts;
    $tsOut = strftime("%A %Y/%m/%d %X", $ts);

    $when = "ago";

    if ($deltaT<0) {
        $deltaT *= -1;
        $when = "from now";
    } 
    $deltaT = $granularity * round($deltaT/$granularity);
 
    if (abs($deltaT)<5) {
        $howMany='';
        $howLong = 'just now';
        $when = '';
        $done=1;
    } 

    if (!$done && $deltaT<60) {
	$howMany = $deltaT;
	$howLong = $howMany==1 ? 'second' : 'seconds';
        $done = 1;
    }

    $deltaT = floor($deltaT/60);
    if (!$done && $deltaT<120) {
	$howMany = $deltaT;
	$howLong = $howMany==1 ? 'minute' : 'minutes';
        $done = 1;
    }
    
    $deltaT = floor($deltaT/60);
    if (!$done && $deltaT<48) {
	$howMany = $deltaT;
	$howLong = $howMany==1 ? 'hour' : 'hours';
        $done = 1;
    }

    $deltaT = floor($deltaT/24);
    if (!$done && $deltaT<15) {
	$howMany = $deltaT;
	$howLong = $howMany==1 ? 'day' : 'days';
        $done = 1;
    }
    $deltaT = floor($deltaT/7);
    if (!$done && $deltaT<90) {
	$howMany = $deltaT;
	$howLong = $howMany==1 ? 'week' : 'weeks';
        $done = 1;
    } 
    $deltaT = $deltaT/52;
    if (!$done) {
	$howMany = $deltaT;
	$howLong = $howMany==1 ? 'year' : 'years';
    }
    if ($howMany>0) 
        $howMany = round($howMany);
    
    return sprintf('<abbr title="%s">%s %s %s</abbr>', $tsOut, $howMany, $howLong, $when);
    echo '</abbr>';
}


function cpf_setlanguage($lang) {
	# Use language to get locale going
	setlocale(LC_MESSAGES, $lang.'_CA.UTF-8');
	setlocale(LC_TIME, $lang.'_CA.UTF-8');
	putenv('LANGUAGE='.$lang.'_CA.UTF-8');
	putenv('LANG='.$lang.'_CA.UTF-8');
	bindtextdomain('messages', $_SERVER['DOCUMENT_ROOT'].'../conf/locale/');
	bind_textdomain_codeset('messages', 'UTF-8');
	textdomain('messages');
}

function nasty_random_id() {
	$rc = ''; 
	while (strlen($rc)<40) {
		$rc .= base_convert( rand(0, getrandmax()), 10, 35 );
	}
	return substr($rc, 0, 40);
}

# until we migrate to php5
if (!function_exists('fputcsv')) {
	function fputcsv(&$handle, $fields = array(), $delimiter = ',', $enclosure = '"') { $str = ''; $escape_char = '\\'; foreach ($fields as $value) { if (strpos($value, $delimiter) !== false || strpos($value, $enclosure) !== false || strpos($value, "\n") !== false || strpos($value, "\r") !== false || strpos($value, "\t") !== false || strpos($value, ' ') !== false) { $str2 = $enclosure; $escaped = 0; $len = strlen($value); for ($i=0;$i<$len;$i++) { if ($value[$i] == $escape_char) { $escaped = 1; } else if (!$escaped && $value[$i] == $enclosure) { $str2 .= $enclosure; } else { $escaped = 0; } $str2 .= $value[$i]; } $str2 .= $enclosure; $str .= $str2.$delimiter; } else { $str .= $value.$delimiter; } } $str = substr($str,0,-1); $str .= "\n"; return fwrite($handle, $str); } 

}

function array_key_multi_sort($arr, $l , $f='strnatcasecmp')
{
	usort($arr, create_function('$a, $b', "return $f(\$a['$l'], \$b['$l']);"));
	return($arr);
}

# obfuscates a text (like, say, an email address) in a way that The Cabal of Perl Programmers have 
# agreed to not harvest off web pages (TINC)
function obfuscate($text) {
    return preg_replace('/[\x00-\x7F]/e', '"&#".ord("$0").";"', $text);
}

function is_url($url){
    $rc=  filter_var($url, FILTER_VALIDATE_URL);
    return $rc;
}

?>
