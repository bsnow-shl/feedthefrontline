<?php

#allow limited support of cli calls to cpf pages:
if ($_SERVER['DOCUMENT_ROOT']=="") {
  $_SERVER['DOCUMENT_ROOT'] = realpath(dirname(__FILE__)."/../docs");
  $_SERVER['HTTP_HOST'] = 'localhost-cli';
  #$_SERVER['SCRIPT_FILENAME'] = 
  #$_SERVER['PHP_SELF']
}



# no web accelerators allowed
if (isset($_SERVER['HTTP_X_MOZ']) && $_SERVER['HTTP_X_MOZ'] == 'prefetch') {
	header('HTTP/1.1 403 Prefetch Forbidden');
    header("Pragma: no-cache");
    echo "Prefetching forbidden.";
	die;
}

# debug time!!


# set up profiling
if ($_REQUEST['phpprofile']) {
	global $_xdebug_trace_file;
	$_xdebug_trace_file = $_SERVER['DOCUMENT_ROOT'].'../logs/xdebug.trace';
	ini_set('xdebug.max_nesting_level', 64);
	if (file_exists($_xdebug_trace_file)) unlink($_xdebug_trace_file);
#	xdebug_start_trace($_xdebug_trace_file);
	xdebug_start_profiling();
}

# bring in the essentials
require_once 'globals.h';
require_once 'phpshould.h';
require_once 'session.h';
require_once 'db.h';
require_once 'templating.h';


# set up CPF globals
{   
	if (substr($_SERVER['REQUEST_URI'],0, 6)=='/admin') {
		# admin is always english
		define('CPF_LANGUAGE', 'en');
	} else {
		# pick current language out of the session
		$_cpf_language = $_SESSION['CPF_LANGUAGE'];

		# override that with the GET, then the server setting, then the first 2 chars out of the URL
		if (substr($_SERVER['REQUEST_URI'],3,1)=='/') {
			$_cpf_language = substr($_SERVER['REQUEST_URI'],1,2);
		}
		if ($_GET['language']) { $_cpf_language = $_GET['language']; }
	
		# failing that look at the browser language
		if (!$_cpf_language) {
			$_cpf_language = substr($_SERVER['HTTP_ACCEPT_LANGUAGE'], 0, 2);
		}
        
		# and vet these through a system-wide config
		if ($Gconfig['i18n']['allowed_languages'] && !$Gconfig['i18n']['allowed_languages'][$_cpf_language]) {
			$_cpf_language = '';
		}
		if (!$_cpf_language) { $_cpf_language = $Gconfig['i18n']['default_language'] ? $Gconfig['i18n']['default_language'] : 'en'; }
		define('CPF_LANGUAGE', $_SESSION['CPF_LANGUAGE'] = $_cpf_language);
		unset($_cpf_language);
	}

	cpf_setlanguage(CPF_LANGUAGE);
	
	# Self: the name of this script
	if (substr($_SERVER['REQUEST_URI'],0,2)=='/~') {
		define('CPF_SELF', $_SERVER['SCRIPT_FILENANE']);
	} else { 
		if ($_SERVER['DOCUMENT_ROOT'][strlen($_SERVER['DOCUMENT_ROOT'])-1] == '/') {
			define('CPF_SELF', substr($_SERVER['SCRIPT_FILENAME'], strlen($_SERVER['DOCUMENT_ROOT'])-1));
		} else {
			define('CPF_SELF', substr($_SERVER['SCRIPT_FILENAME'], strlen($_SERVER['DOCUMENT_ROOT'])));
		}
	}

	# Whether we are running production
	define('CPF_PRODUCTION', $_SERVER['SERVER_PORT'] == 80);
}

# realms:
# We let there be a parallel directory structure in docs/realms/<realmname> that parallels the .html 
# and .data files in docs/. the idea is that the html files can use the .data files under docs.
#
# In other words, /realms/test/foo/bar.html has access to the same .data files if it were at /foo/bar.html
# (both _directory.data and bar.data, btw)


if ($_SERVER['realm']) {
	$_tt[_droot] = $_SERVER['DOCUMENT_ROOT'];
	$_tt[_parts] = preg_split('#/#', $_tt[_droot]);
	$_tt[_real_droot] = join('/', array_slice($_tt[_parts], 0, sizeof($_tt[_parts])-2));

	$_tt[_under_droot] = substr(getcwd(), strlen($_tt[_droot]));

	if (file_exists($_tt[_real_droot].$_tt[_under_droot].'/_directory.data')) {
		eval(read_whole_file($_tt[_real_droot].$_tt[_under_droot].'/_directory.data'));
	}
}

# run directory.data
if (file_exists('_directory.data')) {
	eval(read_whole_file('_directory.data'));
}

# run <fileame>.data from the non-realm part
{
	if ($_SERVER['realm']) {
		$_tt[_fn] = $_tt[_real_droot].CPF_SELF;
		$_tt[_data_fname] = substr($_tt[_fn], 0, strrpos($_tt[_fn], '.')).'.data';
		if (file_exists($_tt[_data_fname])) {
			eval(read_whole_file($_tt[_data_fname]));
		}
	}
}

# run <filename>.data
{
	$_tt[_fn] = $_SERVER['DOCUMENT_ROOT'].CPF_SELF;
	$_tt[_data_fname] = substr($_tt[_fn], 0, strrpos($_tt[_fn], '.')).'.data';
	if (file_exists($_tt[_data_fname])) {
		eval(read_whole_file($_tt[_data_fname]));
	}
}

unset ($_tt);

if ($_GET['template']=='suppress') {
	$_SESSION['template_suppress'] = 1;
}
if ($_SESSION['template_suppress'] || $_SERVER['HTTP_USER_AGENT'] == 'honkydig') {
	template_disable();
	session_destroy();
}

# fire up templating
global $_Template;
$_Template->print_header();

?>
