<?php
require_once "cache.h";

function i18n_delete($class_name, $id) {
  	if (0+$class_name) {
    	$cid = $class_name;
  	} else {
    	$cid = cache_memoize('db_value', 'select class_id from i18n_classes where class_name=?', $class_name);
	}

	$fields = db_multirow('select field_id,field_type,content from i18n_fields join i18n using (field_id) where class_id=? and object_id=? order by precedence', $cid, $id);
	$langs = i18n_get_active_languages();

	db_send('begin');
	db_send('delete from i18n where field_id in (select field_id from i18n_fields where class_id=?) AND object_id=?', $class_id, $id);

	foreach ($langs as $l) {
      foreach ($fields as $f) {
          	$type = unserialize($f['field_type']);
			error_log('type is '.$f['field_type']);
          	if ($type) {
              	$type = $type['type'];
          	} else {
              	$type = $f['field_type']; 
          	}

          	if ($type=='file') {
              require_once 'uploads.h';
				error_log('delete content:'.$f['content']);
              upload_delete($f['content']);
			}
		}
	}

	db_send('delete from i18n where object_id=? and field_id in (select field_id from i18n_fields where class_id=?)', $id,$cid);
	db_send('commit');
	
}


function i18n_get_batch(&$array, $class_name, $id_key, $language='') {
    if ($array===null) return;

    foreach (array_keys($array) as $k) {
		if (!is_null($array[$k][$id_key])) {
                $keys[] = $array[$k][$id_key];
                $keys_index[$array[$k][$id_key]][] = $k;
        }
	}
    
	$answer = i18n_get($class_name, $keys, $language);
	foreach ($answer as $key=>$value) {
		foreach ($keys_index[$key] as $index) {
			$array[ $index ] += $value;
		}
	}
}

function i18n_get($class_name, $id, $language='') {
	if (!$language) { $language = CPF_LANGUAGE; }

	if ($class_name>0) {
		$class_id = &$class_name;
	} else {
		$class_id = cache_memoize('db_value','select class_id from i18n_classes where class_name=?', $class_name);
	}

	if (!$class_id) { return array(); }

	$rc = array();

    if (!is_array($id)) {
	    $query = db_multirow('select field_name,content from i18n_fields,i18n where content != ? and i18n_fields.class_id =? and i18n_fields.field_id = i18n.field_id and language_key=? and object_id = ? order by precedence', '', $class_id, $language, $id);
	    if (sizeof($query)==0) {
			$query = db_multirow("select field_name,content from i18n_fields,i18n where i18n_fields.class_id=? and i18n_fields.field_id = i18n.field_id and language_key='en' and object_id = ? order by precedence", $class_id, $id);
	    }

	    for ($i=0; $i<sizeof($query); $i++) {
        	$rc[$query[$i]['field_name']] = $query[$i]['content'];
	    }
	    return $rc;
    } else {
      	$ids = join(',', $id);
      	$query = db_multirow('select object_id,field_name,content from i18n_fields,i18n where i18n_fields.class_id=? and i18n_fields.field_id = i18n.field_id and language_key=? and object_id in ('.$ids.') order by precedence', $class_id, $language);
     	if (sizeof($query)==0) {
          $query = db_multirow("select object_id,field_name,content from i18n_fields,i18n where i18n_fields.class_id=?  and i18n_fields.field_id = i18n.field_id and language_key='en' and object_id in ($ids) order by precedence",$class_id);
      	}

      	for ($i=0; $i<sizeof($query); $i++) {
      		$rc[$query[$i][object_id]][$query[$i]['field_name']] = $query[$i]['content'];
      	}
		return $rc;
  	}
}

function i18n_set($class_name, $object_id, $language, $content) {
	if ($class_name > 0) {
		$class_id=$class_name;
	} else {
		    $class_id = cache_memoize('db_value', 'select class_id from i18n_classes where class_name=?', $class_name);
	}

    if (!$class_id){
        exit("Unknown i18n class $class_name");
    }

    db_send('begin');
    db_send('delete from i18n where field_id in (select field_id from i18n_fields where class_id=?) and object_id=? and language_key=?', $class_id, $object_id, $language);
    foreach ($content as $key => $value) {
        $field_id = db_value('select field_id from i18n_fields where class_id=? and field_name=?', $class_id, $key);
        if ($field_id) {
            db_send('insert into i18n (field_id, object_id, language_key, content) values (?,?,?,?)', $field_id, $object_id, $language, $value);
        }
    }

    db_send('commit');
}

function i18n_get_active_languages() {
	return cache_memoize('db_multirow', 'select language_key,language_name from i18n_languages where active=1 order by precedence');
}

function  i18n_form_fields($class_name, $id=null, $languages=null) {
    if (0+$class_name>0) {
      $class_id = $class_name;
    } else {
      $class_id = cache_memoize('db_value', 'select class_id from i18n_classes where class_name=?', $class_name);
    }
    

    $pnf = 'pretty_name_'.CPF_LANGUAGE;
	$fields = db_multirow("select * from i18n_fields where class_id=? order by precedence", $class_id);

    if (!$fields) return array();

    if ($languages) { 
		if (is_array($languages)) {
			exit("Not yet implemented.");
		} else {
			$langs = array(   array('language_key' => $languages) );
		}
    } else { 
	    $langs = i18n_get_active_languages(); 
    }

    $form = array();
    foreach ($langs as $l) {
		if (sizeof($langs)>1) {
		    $extra = ' ('.$l['language_key'].')';
		}
		if (!$languages && sizeof($langs)>1) {
			$form[$l['language_name'].'_div'] = array('type'=>'visible',
				'pretty'=> '&mdash;&nbsp;'.$l['language_name']. '&nbsp;&mdash;', 
				'value' =>' ' );
		}
		
	    foreach ($fields as $f) {
	   	    $value = db_value('select content from i18n where field_id=? and language_key=? and object_id=?', $f['field_id'], $l['language_key'], $id);
			if (substr($f['field_type'],0,5)=='call:') {
				$spec = substr($f['field_type'],5);
				$header = substr($spec, 0, strpos($spec, ':'));
				$func = substr($spec, strpos($spec, ':')+1);
				require_once ($header);
				$type = $func();
			} else 	if (strpos($f['field_type'],':')) {
			    $type = unserialize($f['field_type']);
		    } else {
			    $type = array('type'=>$f['field_type']);
		    } 
		    $type['pretty'] = $f['pretty_name'.'_'.$l['language_key']].$extra;
		    $type['help'] = $f['help_text'.'_'.$l['language_key']].$extra;
		    $type['entry_explain'] = $f['entry_explanation'.'_'.$l['language_key']].$extra;
		    $type['required'] = $f['mandatory'];
		    $type['multiple'] = $f['multiple'];
		    if ($f['display_rows']) { $type['rows'] = $f['display_rows']; }
		    if ($f['display_columns']) { $type['cols'] = $f['display_columns']; }
		    if ($f['display_columns']) { $type['size'] = $f['display_columns']; }

            if ($f['presentation_options']) {
              $type['raw_presentation_options'] = $f['presentation_options'];
              $options = split(',', $f['presentation_options']);
              if (in_array('inline', $options)) { $type['style']='inline'; }
              if (in_array('tabular', $options)) { $type['style']='tabular-horizontal'; }
              if (in_array('checkboxes', $options)) { $type['style']='checkboxes'; }
              if (in_array('radio', $options)) { $type['radio']=1; }
            }

            if ($type['multiple']) {
              if (substr($value,0,1)=='[' && substr($value,-1)==']') {
                $value = json_decode($value);
              }
            }
		    $type['default'] = $value;

			$type['name'] = $f['field_name'];
		    if ($type['type']=='richtext' && !$type['default']) { $type['value'] = '<p>&nbsp;</p>'; }
		    $form[$f['field_id'].'_'.$l['language_key']] = $type;
	    }
	}

    return $form;
}

function i18n_form_handle($class_name, $id) {
    if (0+$class_name) {
		$class_id = $class_name;
	} else {
		$class_id = cache_memoize('db_value', 'select class_id from i18n_classes where class_name=?', $class_name);
	}
    $fields = db_multirow('select field_id,field_type from i18n_fields where class_id=? order by precedence', $class_id);
    $langs = i18n_get_active_languages();

    $former_fields = db_hash_multirow('field_id',"select * from i18n where field_id in (select field_id from i18n_fields where class_id=? ) AND object_id=?",$class_id, $id);
    db_send('begin');

	i18n_delete($class_id, $id);
	
    foreach ($langs as $l) {
        foreach ($fields as $f) {
            $type = unserialize($f['field_type']);
            if ($type) {
                $type = $f['type'];
            } else {
                $type = $f['field_type']; 
            }

            if ($type=='file') {
                require_once 'uploads.h';
                $content = $_FILES[$f['field_id'].'_'.$l['language_key']]['upload_id'];
                if (null==$content) {
                    //restore the old field, or delete the field?

                    if ($_REQUEST[$f['field_id'].'_'.$l['language_key'].'_file_cur'] == 'keep') {
                        $content = $former_fields[$f['field_id']]['content'];
                        if (!$content) { 
                            // or maybe there's a hidden form field that was set with the original value
                            // this is a security bug. see ticket #11952
                            $content = $_REQUEST[$f['field_id'].'_'.$l['language_key'].'_file_orig'];
                        }
                    } else {
                        upload_delete($_REQUEST[$f['field_id'].'_'.$l['language_key'].'_file_orig']);
					}
                } else {
                    $orig = $_REQUEST[$f['field_id'].'_'.$l['language_key'].'_file_orig'];
					if ($orig) { upload_delete($orig); }
                    db_send('update uploads set referring_table=?,referring_id=? where upload_id=?', 'i18n:'.$class_id, $id, $content);
                }
            } else {
                $content = $_REQUEST[$f['field_id'].'_'.$l['language_key']];
                if (is_array($content)) { $content = json_encode($content); }
            }
            db_send('insert into i18n (field_id, language_key, object_id, content) values (?,?,?,?)', $f['field_id'], $l['language_key'], $id, $content);
        }
    }

    db_send('commit');
}



# overhaul below
function i18n_get_current_content_objid($content_id) {
	return db_value('select object_id from i18n_content_map where content_id=? and posting_datetime < CURRENT_TIMESTAMP and (takedown_datetime is null or CURRENT_TIMESTAMP < takedown_datetime)', $content_id);
}

function i18n_get_current_content($content_id_or_name) {
  if ($content_id_or_name>0) {
    $content_id = $content_id_or_name;
    $class_id = cache_memoize('db_value','select class_id from i18n_content where content_id=?', $content_id_or_name);
  } else {
    $row = cache_memoize('db_row', 'select class_id,content_id from i18n_content where short_name=?', $content_id_or_name);
    $content_id = $row['content_id'];
    $class_id = $row['class_id'];
  }

  if ($_SESSION['content']['preview'][$content_id]>0) {
	$objid = $_SESSION['conetent']['preview'][$content_id];
	unset($_SESSION['content']['preview'][$content_id]);
	apache_note('page_uncacheable', 1);
  } else {
	$objid = i18n_get_current_content_objid($content_id);
  }
	
  if ($content_id && $objid)  {
    return cache_memoize('i18n_get', $class_id, $objid, CPF_LANGUAGE);
  } else {
    return array();
  }
}


?>
