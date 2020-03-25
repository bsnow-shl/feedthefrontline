<?php

function email_get_valid_classes() {
	$rc = config_get('email','classes');
	if ($rc) return $rc;

	return array (
			'POP3' => array( 'require' => 'password'),
			'forwarding' => array( 'require' => 'to_addr', 'help' => 
						'To forward email to multiple destinations, separate them with a comma.
						<br/>
						 To force-deliver mail locally, prefix the address with a \ (e.g.: <tt>\bob</tt>)'
						 ),
			'mailing list' => array( 'require' => 'moderator,memberlist')
	);
}

function email_form_for_class($class) {
    global $email_valid_classes;
    $form = array(
        'domain' => array('type' => 'menu', 'required' => 1,
                'sql' => 'select email_domain_id as key, domain_name as value from email_domains'),
        'from_address' => array('type'=>'text', 'required' => 1)
    );

    $extra = $email_valid_classes[$class];
    $required = preg_split('/,/', $extra['require']);
    if (in_array('moderator', $required)) {
        $form['moderator'] = 'email,required';
        $form['moderation_rule'] = array('type'=>'menu',
				'menu'=>array('moderated'=>'only moderators can post', 
						'open' => 'anyone can post'),
				'required' => 1,
				'default' => 'moderated');
    }
    if (in_array('password', $required)) {
        $form['password'] = 'text,required';
    }
    if (in_array('to_addr', $required)) {
        $form['to_address'] = 'text,required';
    }
    if (in_array('memberlist', $required)) {
	$form['members'] = array('type'=>'bigtext');
    }

    return $form;
}

function email_get_system_files() {
    global $Gconfig;

    $rc = array();

    # aliases
    $query = db_multirow("select from_addr,to_addr,domain_name from email_accounts,email_domains  where class='forwarding' and email_accounts.email_domain_id = email_domains.email_domain_id");

    foreach ($query as $q) {
            $from = $q['from_addr'];
            $to = $q['to_addr'];
            $aliases[$q['domain_name']][$from][] = $to;
    }


    # passwd for pop3 accounts
    $query = db_multirow("select from_addr,password,domain_name from email_accounts,email_domains  where class='POP3' and email_accounts.email_domain_id = email_domains.email_domain_id");

    foreach ($query as $q) {
            $from = $q['from_addr'];
            $crypted = crypt($q['password']);
            $passwd[$q['domain_name']] .= "$from:$crypted\n";

            # if there's an alias for them, tack on "\userid" to force local delivery
	    if ($aliases[$q['domain_name']][$from]) {
		    $aliases[$q['domain_name']][$from][] = '\\'.$from;
	    }
    }
    
    $query = db_multirow("select from_addr,moderator,domain_name from email_accounts,email_domains  where class='mailing list' and email_accounts.email_domain_id = email_domains.email_domain_id");

    foreach ($query as $q) {
            $from = $q['from_addr'];
            $moderator = $q['moderator'];
	    $maillist = $Gconfig['email']['maillist_prefix'].$from;

            $aliases[$q['domain_name']][$from][] = "|/usr/lib/ecartis/ecartis -s $maillist";
            $aliases[$q['domain_name']][$from.'-request'][] = "|/usr/lib/ecartis/ecartis -r $maillist";
            $aliases[$q['domain_name']][$from.'-repost'][] = "|/usr/lib/ecartis/ecartis -a $maillist";
            $aliases[$q['domain_name']][$from.'-admins'][] = "|/usr/lib/ecartis/ecartis -admins $maillist";
            $aliases[$q['domain_name']][$from.'-moderators'][] = "|/usr/lib/ecartis/ecartis -moderators $maillist";
            $aliases[$q['domain_name']][$from.'-bounce'][] = "|/usr/lib/ecartis/ecartis -bounce $maillist";
    }



    # grind out our files
    if ($aliases) {
	    foreach ($aliases as $domain => $targets) {
		if (!$_SERVER['DEVEL']) {
		    $file = "/etc/mail/domains/$domain/aliases";
		} else {
		    $file = $_SERVER['DOCUMENT_ROOT']."../var/email-$domain-aliases";
		}
		$contents = '';
		foreach ($targets as $from => $tos) {
		    $contents .= "$from: ".join(',',$tos)."\n";
		}
		$rc[] = array('file' => $file, 'contents' => $contents);
	    }
    }
    if ($passwd) {
	    foreach ($passwd as $domain => $contents) {
		if (!$_SERVER['DEVEL']) {
		    $file = "/etc/exim/$domain/passwd";
		} else {
		    $file = $_SERVER['DOCUMENT_ROOT']."../var/email-$domain-passwd";
		}
		$rc[] = array('file' => $file, 'contents' => $contents);
	    }
    }
    return $rc;
}

function email_get_maillist_userfile($list) {
    global $Gconfig;
    $maillist = $Gconfig['email']['maillist_prefix'].$list; 
    $file = "/var/lib/ecartis/lists/$maillist/users";
    if ($_SERVER['DEVEL']) {
        $file = $_SERVER['DOCUMENT_ROOT']."../var/ecartis-$maillist-users";
    }
    return $file;
}


function email_get_maillist_files($list, $users) {
    global $Gconfig;
    $list_row = db_row('select * from email_accounts where from_addr=?', $list);

    $file = email_get_maillist_userfile($list);

    $contents = $list_row['moderator']." : |ECHOPOST|MODERATOR|SUPERMODERATOR|ADMIN|\n";
    foreach ($users as $e) {
	    if ($e) {
		$contents .= "$e : |ECHOPOST|\n";
	    }
    }

    $rc[] = array('file' => $file, 'contents' => $contents);
    return $rc;
}


