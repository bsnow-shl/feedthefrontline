<?php

require_once 'config.h';

function db_config($which='') {
    global $Gdb, $Gconfig;
    if ($Gdb) {
	    $Gconfig['db'] = $Gdb;
	    $Gdb=null;
    }
    if ($which) 
        return $Gconfig['db'][$which];
    else
        return $Gconfig['db'];
}

# Retrieves a database connection.
function db_open($which = 'default') {
	static $conns;
	if ($conns[$which]) {
		return $conns[$which];
	}

	$conf = db_config();
	if ($conf['dbname']) {
		$db = $conf['dbname'];
	} else {
		$db = join(explode('.', $_SERVER['SERVER_NAME']), '_');
	}

	if ($conf['dbport']) {
		$db .= " port=".$conf['dbport'];
	}
	if ($conf['dbhost']) {
		$db .= " host=".$conf['dbhost'];
	}
	if ($conf['dbuser']) {
		$db .= " user=".$conf['dbuser'];
	}
        if ($conf['dbpassword']) {
                $db .= " password=".$conf['dbpassword'];
        }

	$conns[$which] = $conn = pg_pconnect("dbname=$db");
	if (!$conn) {
		error_log("Unable to connect to $db while processing ".
				$_SERVER['SERVER_NAME'].$_SERVER['REQUEST_URI']);
	}
	return $conn;
}


# Sends off a transaction, possibly parsing it for placeholders.
function db_send($sql) {
	static $beginCount=0;

	# push around the arguments
	$numargs = func_num_args()-1;
	$arg_list = func_get_args();
	$sql = array_shift($arg_list);
	while ($numargs && is_array($arg_list[0])) {
		$arg_list = $arg_list[0];
		$numargs = sizeof($arg_list);
	}
	

	$time = 0;
	if ($_REQUEST['sqldebug'] || $_SERVER['sqldebug']) {
		$time = microtime(true);
	}

	# identify placeholders (try a cache first)
	global $_db_placeholder_cache;
	if (db_get('no_placeholder_cache') || !isset($_db_placeholder_cache[$sql])) {
		$placeholders = array();
		$quote = 0;
		$max = strlen($sql);
		for ($i = 0; $i < $max; ++$i) {
		  if ($sql[$i] == "'" && $sql[$i-1] != '\\') {
		    if (!$quote) {
		      $quote = 1;
		    } elseif ($quote == 1) {
		      $quote = 0;
		    }
		  } elseif ($sql[$i] == '"') {
		    if (!$quote) {
		      $quote = 2;
		    } elseif ($quote == 2) {	
		      $quote = 0;
		    }	
		  } elseif ($sql[$i] == '?') {
		    if (!$quote) {
		      array_push($placeholders, $i);
		    }
		  }
		}
		if (!db_get('no_placeholder_cache')) {
		$_db_placeholder_cache[$sql] = $placeholders;
		}
	} else {
		# Cool! Got it from cache.
		$placeholders = $_db_placeholder_cache[$sql];
	}

	# substitute placeholders
	$parms = array();
	for ($i = 0; $i < $numargs; $i++) {
		if (is_array($arg_list[$i])) {
			while (list($dummy,$parm) = each ($arg_list[$i])) {
				array_push($parms, $parm);
			}
		} else {
			$parms[] = $arg_list[$i];
		}
	}
	if (sizeof($parms) < sizeof($placeholders)) {
		die("<p><b>SQL Query (".$sql.") contains ".sizeof($placeholders)." placeholders but ".sizeof($parms)." was passed</b></p>");
	}
	if (sizeof($placeholders) > 0) {
		$newsql = substr($sql, 0, $placeholders[0]);
		for ($i = 0; $i < sizeof($placeholders) - 1; ++$i) {
			$newsql .= _db_quote($parms[$i]) . substr($sql, $placeholders[$i] + 1, $placeholders[$i + 1] - $placeholders[$i] - 1);
		}
		$sql = $newsql. _db_quote($parms[$i]) . substr($sql, $placeholders[$i] + 1);
	 } else {
		# no substitution to do, so leave it alone
	 }

	# if this is a begin or commit, we might eat it depending on whether there's been other begins or commits
	if ($sql=='begin') {
		if ($beginCount++) {
			$skip_query_exec = 1;
		}
	}
	if ($sql=='commit') {
		if (--$beginCount) {
			$skip_query_exec=1;
		}
	}
	if ($sql=='rollback') { $beginCount = 0; }

	# run the query
	if (!$skip_query_exec) {
		$conn = db_open();
		if ($conn == 0) { return 0; }

		$rc = pg_exec($conn, $sql);

		$rows_affected  = $rc ? pg_numrows($rc) : 0;
		db_set('last_rowcount', $rows_affected);

		$error = pg_errormessage($conn);
		if ($error) {
			error_log("$error for $sql");
		}

		if ($_REQUEST['sqldebug'] || $_SERVER['sqldebug']) {
                    if ($_REQUEST['sqldebug'] != 2 || (microtime(true)-$time)>0.05) {
			static $log;
			if (!$log) {
				$log = fopen($_SERVER['DOCUMENT_ROOT'].'../logs/sql.log', 'a+');
			}
			fwrite($log, sprintf("%.5f\t%d\t%s\t%s\n", microtime(true) -$time, $rows_affected, $_SERVER[REQUEST_URI], str_replace("\n", ' ', $sql)));

			global $_sql_debug;
			$_sql_debug[] = array(
				'query' => $sql,
				'parameters' => $parms,
				'time' => microtime(true)-$time,
				'rows' => pg_numrows($rc),
				'error' => $error
			);
                    }
		}
	} else {
		if ($_REQUEST['sqldebug'] || $_SERVER['sqldebug']) {
			global $_sql_debug;
			$_sql_debug[] = array('query' => $sql.'  -- NESTED -- NOT RUN' );
		}
	}

	return $rc;
}


# takes the result of db_send
function db_last_oid($result) {
    return pg_last_oid($result);
}


# builds an update out of the given parameters, with a simple where clause.
function db_update($table, $column, $value, $array) {
	$sql = "";
	foreach ($array AS $c => $v) {
		if ($sql) {
			$sql .= ',';
		}
		$sql .= $c.'='._db_quote($v);
	}

	return db_send("UPDATE $table SET $sql WHERE $column="._db_quote($value));
}


function db_insert_or_update($table, $column, $value, $hash) {
	if (db_value("select count(*) from $table where $column=?", $value)>0) {
		return db_update($table, $column, $value, $hash);
	} else {
		return db_insert_hash($table, $hash+array($column=>$value));
	}
}


# builds a simple insert for one row into the given table
function db_insert_hash($table, $hash) {
	foreach ($hash as $key=>$value) {
		$hash[$key] = _db_quote($value);
	}	
	return db_send('INSERT INTO '.$table.' ('.join(',',array_keys($hash)).')  values ('.join(',',array_values($hash)).')');
}

# get a single value from a database row
function db_value($sql) {
	if (func_num_args()>1) {
		$arg_list = func_get_args();
		array_shift($arg_list);
		$result = db_send($sql, $arg_list);
	} else {
		$result = db_send($sql);
	}

	if ($result == false) { return NULL; }

	$rows = pg_numrows($result);
	if ($rows==0) {
		return NULL;
	} elseif ($rows>1) {
		$msg = "Got $rows rows back, instead of only one, while executing $sql";
		error_log($msg);
		user_error($msg);
		return NULL;
	}
	$rc = pg_result($result, 0, 0);
	pg_freeresult($result);
	return $rc;
}

# get multiple values from a database query
function db_multirow($sql) {
  $arg_list = func_get_args();
  return _db_hash_multirow_guts(false,null,$sql, $arg_list);
}

#get multiple values from a database query
function db_hash_multirow($key_column, $sql) {
  $arg_list = func_get_args();
  return _db_hash_multirow_guts(false,$key_column, $sql, $arg_list);
}

# get multiple values from a database query, limit the number of response rows
function db_paged_multirow($sql) {
  $arg_list = func_get_args();
  return _db_hash_multirow_guts(true,null,$sql, $arg_list);
}

#get multiple values from a database query, limit the number of response rows
function db_paged_hash_multirow($key_column, $sql) {
  $arg_list = func_get_args();
  return _db_hash_multirow_guts(true,$key_column, $sql, $arg_list);
}


function _db_hash_multirow_guts($paged,$key_column, $sql, $arg_list) {
  if ($paged) {
    if ($_REQUEST['query_limit']) {
      $query_limit = $_REQUEST['query_limit'];
    } else {
      $query_limit = 9;
    }
    $sql .= " limit "._db_quote($query_limit);
    if ($_REQUEST['query_offset']) {
      $sql .= " offset "._db_quote($_REQUEST['query_offset']);
    }
  }

	if (count($arg_list)>1) {
		$result = db_send($sql, array_splice($arg_list, ($key_column)?2:1, count($arg_list)));
	} else {
		$result = db_send($sql);
	}

	if ($result == false) { return NULL; }

	$rows = db_get('last_rowcount');
	if ($rows==0) {
		return array();
	}

	$max = db_get('maxrows');
	if ($max) {
		$rows = min($max, $rows);
	}

	$rc = array();
	for ($i=0; $i<$rows; $i++) {
	  if ($key_column) {
		$rc[pg_result($result, $i, $key_column)] = pg_fetch_array($result, $i, PGSQL_ASSOC);
	  } else {
		$rc[$i] =  pg_fetch_array($result, $i, PGSQL_ASSOC);
	  }
	}
	pg_freeresult($result);
	return $rc;
}

# fetches a 1-row result, and returns an array with each field keyed by name
function db_row($sql) {
	if (func_num_args()>1) {
		$arg_list = func_get_args();
		array_shift($arg_list);
		$result = db_send($sql, $arg_list);
	} else {
		$result = db_send($sql);
	}

	if ($result == false) { return NULL; }

	$rows = pg_numrows($result);
	if ($rows==0) {
		return NULL;
	} elseif ($rows>1) {
		$msg = "Got $rows rows back, instead of only one, while executing $sql";
		error_log($msg);
		user_error($msg);
		return NULL;
	}
	$rc = pg_fetch_array($result, 0, PGSQL_ASSOC);

	pg_freeresult($result);
	return $rc;
}

# fetches a 1-column result, and returns an array with each column in order
# if you ask for a 2-column result, it looks for key + value
function db_array($sql) {
	if (func_num_args()>1) {
		$arg_list = func_get_args();
		array_shift($arg_list);
		$result = db_send($sql, $arg_list);
	} else {
		$result = db_send($sql);
	}

	if ($result == false) { return NULL; }
	if (pg_numfields($result) != 1 && pg_numfields($result) != 2) { error_log("$sql was only supposed to return one field"); return NULL; }
	$rows = pg_numrows($result);
	if ($rows==0) {
		return array();
	}

	$rc = array();
	if (pg_numfields($result) == 1) {
		for ($i=0; $i<$rows; $i++) {
			array_push($rc, pg_result($result, $i, 0));
		}	
	} else {
		for ($i=0; $i<$rows; $i++) {
			$rc[ pg_result($result, $i, 0) ] = pg_result($result, $i, 1);
		}		
	}
	pg_freeresult($result);
	return $rc;
}


# Given the result of db_send, returns the number of rows affected.
function db_tuples($result) {
	return pg_affected_rows($result);
}


# gets a new value from the (named) database sequence.  
function db_newid($seq) {
	$seq .= "_id";
	if (!db_value("SELECT relname FROM pg_class WHERE relkind='S' and relname='$seq'")) {
		db_send("CREATE SEQUENCE $seq START 10000");
	}

	return db_value("SELECT nextval('$seq')");
}

# sets a config variable for the duration of this visit
function db_set($key, $value) {
	global $Gconfig;
    $rc = $Gconfig['db'][$key];
	if (is_null($value)) {
		unset( $Gconfig['db'][$key]);
	} else {
		$Gconfig['db'][$key] = $value;
	}
    return $rc;
}

# gets a config variable set by db_set
function db_get($key) {
	global $Gconfig;
	return $Gconfig['db'][$key];
}

function db_errormessage($result=null) {
	if (!defined($result)) {
		return pg_last_error(db_open());
	} else {
		return pg_result_error($result);
	}
}

# does some quoting magic
function _db_quote ($in_str, $forcestring=false) {
	if (is_null($in_str)) { return 'null'; }

	$str = strval($in_str);
	static $magic = null;
	if ($magic === null) { $magic = get_magic_quotes_gpc(); };

	if ($magic) {
		$str = stripslashes($str);
	}
	if ( 0+$str === $str && !$forcestring) {
		return $str;
	} else {
		return "'".str_replace("'", "''", $str)."'";
	}
}

# guess a pkey given the table name
function db_pkey_for_table($table_name) {
	$pkey_guess = $table_name;
	if (substr($table_name,-1)=='s') {
		$pkey_guess = substr($table_name, 0, -1);
	}
	return $pkey_guess.'_id';
}

# use this for when very sensitive stuff is done, and you don't want it logged
function db_debug_clear_log() {
	global $_sql_debug;
	$_sql_debug = array(
		array('query' => 'Log reset, probably due to paranoia')
	);
}

function _cpf_error_handler($errno, $errstr, $errfile='', $errline=0, $context=null) {
	#explicitly excluded errors:
	if ($errno >= E_STRICT || ($errno == E_NOTICE && preg_match('/^(Constant|Trying to get property|Undefined property|Undefined offset|Undefined variable|Undefined index|Use of undefined constant)/', $errstr))) return;

	#no reporting if the programmer used '@':
	if (ini_get('error_reporting')==0) return;
	if ($errno!=E_NOTICE && $errno < E_DEPRECATED) {
		error_log("$errstr $errfile $errline");
		ob_start();
		show_vars(FALSE);
		print_a($_SERVER);
		$context_string .= ob_get_clean();

		ob_start();
		print_r(debug_backtrace());
		$context_string .= "\n\nBacktrace:\n\n".htmlspecialchars(ob_get_clean());

		db_send('rollback');
		db_send('begin');
		$error_id = db_newid('error_id');
		db_send('insert into errors(error_id,user_id,level,message,file,file_line,context) values (?,?,?,?,?,?,?)',
				                        $error_id, $_SESSION['user_id'], $errno, $errstr, $errfile, $errline, $context_string);
		db_send('commit');

		if ($errno != E_USER_NOTICE) {
			exit('An internal error (#$error_id) has ocurred. It has been reported to site management staff. Please try again, or wait until we can figure out the problem.');
		}
	} else {
		error_log("${_SERVER['REQUEST_URI']} $errno $errstr $errfile $errline");
	}
}

if (!$_SERVER['DEBUG'] && !$_GET['phpprofile'] && substr($_SERVER['REQUEST_URI'],0,7)!='/admin/') {
	 set_error_handler('_cpf_error_handler');
}

?>
