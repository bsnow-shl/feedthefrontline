<?php

$Gconfig = array(
	'db' => !$_SERVER['DEVEL'] ? array(
                'dbname' => 'unconfigured',
        ) : array(
      // from ini
    ),


	'site' => array(
		'short_name' => 'scrotate-web',
//		'home' => 'http://csic2.tinyplanet.ca/', // note trailing slash
//      'pretty_name'=> 'Sanyo Canada - Presentation Technologies', 
//		'owner' => 'CSIC',
//		'owner-email' => 'information@csic-scci.ca',
	),

	// split errors off into their own table in auto-rendered forms
	'form' => array('separate_errors'=>1),
	
	'events' => array(
	  'availability_criteria' => array(
		array( 'title'=>'Always show',
			'header'=> 'events.h',
			'function'=>'event_true',
			'show_to_public' => 1
		),
	   ),
	),

    'salt' => 'unimaginative',

/*  Some sample configurations, commented out

	// turn on content management
	'content' => array (
		'tree_root' => 10002,
		'link_asset_document_category' => 10000,
        'image_asset_document_category' => 10001,
        'tree_root' => '10002',
		'content_css' => '/style.css',
		'css_styles_div' => 'blockquote'
        'template_files' => array( '/en/_template.html', '/fr/_template.html'),
	),

	// permit french and english
	'i18n' => array('allowed_languages' => array('en'=>"English",'fr'=>"Fran&ccedil;ais"),
			default_language => 'en'
	),

    'newsletter' => array(
        'outgoing_root' => 'outgoing',
        'editions' => array('en'=>'English', 'fr'=>'French', 'test'=>'Testing'),
        'system_lists_callback' => ''
    ),

	'users' => array (
		// create your own extensions to the edit form
		'user_admin_edit_callback' => 'csic_user_admin_edit_form',

		// with these fields
		'fields_callback' => 'csic_user_fields',

		// add stuff into the user search form
		'user_admin_search_callback' => 'csic_user_search_form',

		// use a search form no matter what
		'force_search' => 1,

		// where to redirect after creating a new user
		'post_register' => '/members/',

		// collect payment for registrations
		'membership_payment_header' => 'itsmf.h',
		'membership_payment_function' => 'itsmf_membership_notify',

        //account registration options:
        
        'account' => array(
            //verification requires a valid email address, and checks by sending 
            //  a message to that address and waiting for the code to come back
            //  This requires items in the content management system:
            //  an object called 'regemail' with fields:
            //  'body':  in which 
            //           @@email@@ is replaced with the "to" address,
            //           @@maillink@@ is replaced with the URL to load that will verify the address
            //           @@sitename@@ is replaced with config_get('site','home') from this file
            //           @@reg_code@@ is replaced with a code that can be entered on the page
            //              /account/verifyemail.html to verify the address
            //  'subject'
            //  'attachment'
            'no_registration'=>1,
            'verify_new_email'=>, 
            'invite_only'=>1),
	),

    'shopping' => array(
        'suppress_cart_reporting'=>1,
        'small_geometry' => 75,
        'medium_geometry' => '300x175',
        'admin_category_picker' => array('projector'=>'10002','projector accessories'=>'10018'),
    ),
*/
);

if (!$Gconfig['db']) {
    $_parsed_ini = parse_ini_file($_SERVER['DOCUMENT_ROOT'].'/../conf/cpfdb.ini', true);
    $_dbname = $_parsed_ini['default']['defaultdb'];

    if (!$_dbname) { $_dbame = config_get('site','short_name'); }
    if (file_exists( $_ENV['HOME'].'/.cpfdb.ini' )) {
        $_new_parsed_ini = parse_ini_file($_ENV['HOME'].'/.cpfdb.ini', true);
        $Gconfig['db'] = $_new_parsed_ini[ $_dbname ];
    }
    if ($_parsed_ini==FALSE) { exit("No file ~/cpfdb.ini"); }
    if (!$Gconfig['db']) {
      $Gconfig['db'] = $_parsed_ini[ $_dbname ];
    }
}

?>
