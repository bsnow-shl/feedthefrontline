<?

function _cache_connect() {
	if (!$_SERVER['DEVEL']) {
		$conn = @memcache_pconnect('memcache.tp', 11211);
		return $conn;
	}
	return FALSE;
}

function _cache_key_for(&$key) {
	static $dbname;
	if (!$dbname) {
		$dbname = db_get('dbname');
	}
	return $dbname.'%%'.$key;
}

function cache_get(&$key) {
	if (!extension_loaded('memcache')) { return FALSE; }
	$cache = _cache_connect();
	if (!$cache) return FALSE;
	return @memcache_get($cache, _cache_key_for($key));
}

function cache_set(&$key, &$value) {
	if (!extension_loaded('memcache')) { return FALSE; }
	$cache = _cache_connect();
	if (!$cache) return FALSE;
	return @memcache_set($cache, _cache_key_for($key), $value);
}

function cache_stats() {
	if (!extension_loaded('memcache')) { return FALSE; }

	$cache = _cache_connect();
	if (!$cache) return FALSE;
	return memcache_get_stats($cache);
}

function cache_memoize() {
	$args = func_get_args();
	$key = serialize($args);

	$rc = cache_get($key);
	if ($rc !== FALSE && !is_null($rc)) {
		return $rc;
	}
	$func = array_shift($args);
	$rc = call_user_func_array($func, $args);
	cache_set($key, $rc);
	return $rc;
}

function cache_get_extended_stats() {
	if (!extension_loaded('memcache')) { return FALSE; }
	$cache = _cache_connect();
	if (!$cache) return FALSE;
	$rc = memcache_get_extended_stats($cache);
	$rc2 =  array_values($rc);
	return $rc2[0];
}


?>
