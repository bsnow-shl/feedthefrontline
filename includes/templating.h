<?php

global $_template_first_half;

class CpfTemplate {
	var $_specified_template;
	var $_template_search_path;
	var $_template_disable;
	var $_template_data;
	var $_template_cut_index;
	var $_result_cut_index;
	var $_ob_started;

	# Returns the matching filename in the document root, searching
	# backwards from the location of the current PHP script.
	# Parameters: 
	#  1. the file to find, and 
	#  2. 'true' if the search should be only in the local directory
	function template_find($file, $current_dir_only=0) {
		if ($current_dir_only) {
			$try = $_SERVER['DOCUMENT_ROOT'].$this->_template_search_path[0].'/'.$file;
			if (file_exists($try)) {
				return $try;
			}
			return null;
		}
		foreach ($this->_template_search_path as $path) {
			$try = $_SERVER['DOCUMENT_ROOT'].$path.'/'.$file;
			if (file_exists($try)) {
				return $try;
			}
		}
		return null;
	}

	function template_include($file, $current_dir_only=0) {
		$try = template_find($file, $current_dir_only);
        if (!$try && file_exists($file)) { $try = $file; }
		if ($try == null) {
			return;
		}

		$d = substr($try, 0, strrpos($try, '.')).'.data';
		if (file_exists($d)) {
			eval(read_whole_file($d));
		}

		eval ('?>'.read_whole_file($try).'<?');
	}


	function template_disable() {
		$this->_template_disable = 1;
		if (defined($this->_ob_started) && $this->_ob_started) {
			ob_end_flush();
		}
	}

	function template_is_disabled() { 
		return $this->_template_disable ? 1 : 0;
	}

	function template_specify($file) {
	    $this->_specified_template = $file;
	}

	function print_header() {
		if ($this->_template_disable) return;
		$script = CPF_SELF;
		$script_path = explode('/', $script);

	    $template_file = $this->_specified_template;
	    if (!$template_file) {
          for ($i = count($script_path)-1; $i >= 1;  $i--) {
              $directory = join('/', array_slice($script_path, 0, $i));
              $this->_template_search_path[] = $directory;
              $template_file = $_SERVER['DOCUMENT_ROOT'].$directory.'/_template.html';
              if (file_exists($template_file)) {
                $template_file = $template_file;
                break;
              }
          }
	    }
	    if ($template_file && is_readable($template_file)) {
          $fh  = fopen($template_file, 'r');
          while (!feof($fh)) {
              $this->_template_data .= fread($fh, 65536);
          }
          $this->_template_cut_index = strpos($this->_template_data, '##PART##');
          if ($this->_template_cut_index === FALSE) {
              print "Error: there's no ##PART## in $template_file";
              die;
          }

          // run the first half of the template
          ob_start();
          eval ('?>'.substr($this->_template_data, 0, $this->_template_cut_index).'<?');
          global $_template_first_half;
          $_template_first_half = ob_get_contents();
          ob_end_clean();

          ob_start("_template_find_keywords", 65536);
          $this->_ob_started = true;

          return;
	    }

	}

	function print_footer() {
		eval ("?>".substr($this->_template_data, $this->_template_cut_index+8)."<?");
	}
}


function template_find($file, $current_dir_only=0)  {
	global $_Template;
	return $_Template->template_find($file, $current_dir_only);
}

function template_include($file, $current_dir_only=0)  {
	global $_Template;
	return $_Template->template_include($file, $current_dir_only);
}

function template_disable() {
	global $_Template;
	return $_Template->template_disable();
}

# specifies a certain file to be used as _template.html instead of the usual
function template_specify($file) {
	global $_Template;
	return $_Template->template_specify($file);
}

// while rendering foo.html, template_include_piece('bar') will look for _bar.html then foo_bar.html, and include them if found
function template_include_piece($piece_name, $current_dir_only=0) {
	global $SCRIPT_FILENAME;

	$pathinfo = pathinfo($_SERVER['SCRIPT_FILENAME']);
	$filename = $pathinfo['basename'];
	$try_files = array(
		"_${piece_name}.html", 
		substr($filename, 0, strlen($filename)-strlen($pathinfo['extension'])-1).'_'.$piece_name.'.'.$pathinfo['extension']
	);
	
	foreach ($try_files AS $try) {
		if (template_find($try,$counter > 0)) {
			echo '<div class="${piece_name}_holder">';
			template_include($try, $counter>0);
			echo '</div>';
		}
		$counter++;
	}
}

function _template_find_keywords(&$content, $whatis) {
  static $_template_keywords=null;
  
  if (!$_template_keywords) {
    // look for keywords; they are between the start of the content and the first -->
	$start = 0;
    $end = strpos($content, '-->');
	if (!$end) {
		$heading = $content;
	} else {
		$heading = substr($content, 0, $end);
	}

    // keywords look like "foo>...</foo"
	$r = preg_match_all("#(\w+)>(.*)</\\1#ismU", $heading, $_template_keywords);
	foreach ($_template_keywords[1] as $k=>$v) {
		$keys[strtolower($v)] = $_template_keywords[2][$k];
		$keys[strtoupper($v)] = strip_tags($_template_keywords[2][$k]);
	}
  }
    
  if ($whatis & PHP_OUTPUT_HANDLER_START) {
    global $_template_first_half;
	$rc=preg_replace('/@@(\w+)@@/em', "\$keys['\\1']", $_template_first_half. $content);
	$_template_first_half = '';
  } else {
	$rc=preg_replace('/@@(\w+)@@/em', "\$keys['\\1']", $content);
  }
  
  // maybe there's more post-processing
  $steps = config_get('template', 'processing-rules');
  if (!$steps) return $rc;

  $start_search = config_get('template', 'processing-rules-start-search');  
  if ($start_search && $idx = strrpos($rc, $start_search)) {
      // great
  } else {
      $idx = strpos($rc, '<body')+5;      
  }
  if ($idx) {
    $rc_pre = substr($rc, 0, $idx);
    $rc = substr($rc, $idx);
    foreach ($steps as $regex=>$function) {
       $rc2 = preg_replace_callback($regex, $function, $rc);
       $rc = $rc2;   
    }
    $rc = $rc_pre.$rc;
  }
  return $rc;
}

global $_Template;
$_Template = new CpfTemplate;

if (substr($_SERVER['REQUEST_URI'],0,5)=='/blog') {
	template_disable();
}
?>
