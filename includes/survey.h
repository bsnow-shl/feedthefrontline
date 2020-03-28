<?

require_once 'users.h';
require_once 'i18n.h';

function survey_get_form_for_survey($sid) {
		$survey = db_row('select * from surveys where survey_id=?', $sid);
		$questions = db_multirow('select * from survey_questions where survey_id=? order by display_order', $sid);
		i18n_get_batch($questions, 'Survey question', 'survey_question_id');
		foreach ($questions as $q) {
			$item = array(
					'pretty'=> nl2br($q['question']),
                    'default' => $q['question_type']=='menu' ? -157157116 : '',
					'type' => $q['question_type'],
					'required' => $q['required'],
			);
			if ($q['question_type'] == 'bigtext') { $item['cols'] = 25; $item['rows'] = 4; }
			if ($q['question_type'] == 'text' || $q['question_type']=='email') { $item['size'] = 25; }
			if ($q['question_type'] == 'flaglist') {
				$menus = unserialize($q['question_type_extra']);
                $item['menu'] = $menus[CPF_LANGUAGE];
                $item['multiple']=1;
                $item['type']='menu';
			}
			if ($q['question_type'] == 'menu') {
				$menus = unserialize($q['question_type_extra']);

				$item['menu'] = $menus[CPF_LANGUAGE];
				$item['radio'] = 1;
			}
			$form[ 'q_'.$q['survey_question_id'] ] = $item;
		}
		$form['vote'] = array('type'=>'submit','value'=>_('Submit'));

        // bake up a template if one isn't provided
        $survey_details = i18n_get('Survey preamble', $sid);
        if ($survey_details['form']) { 
            $template = $survey_details['form'];
        } else {
            if ($survey['survey_type']=='multiple-choice-exam') {
                $template = '';
                foreach ($questions as $q) {
                        $template .= "<h4>{$q['question']}</h4><p>@@q_{$q['survey_question_id']}@@</p><p>##q_{$q['survey_question_id']}##</p>";
                }
                $template .= '<br/><br/>@@vote@@';
            } else if ($survey['survey_type']=='popquiz') {
                    $template = "<table class=\"padded survey\"><tr><th>Survey: {$questions[0][question]}</th></tr><tr><td>@@q_{$questions[0][survey_question_id]}@@<br/>@@vote@@</td></tr></table>";
            } else {
                $template = '<table class="padded survey">';
                foreach ($questions as $q) {
                    $required = $q['required'] ? '&nbsp;<span class="required">*</span>' : '';
                    $template .= "<tr><th class=\"question\">{$q['question']}:$required</th><td>@@q_{$q['survey_question_id']}@@</td></tr><tr><td colspan=\"2\">##q_{$q['survey_question_id']}##</td></tr>";
                }
                $template .= '<tr><td>@@vote@@</td></tr></table>';
            }
            if ($sid==20005) $template .= '<p>(*) Required field</p>';
        }

		return array($form, $template);
}

function survey_check_answers($sid) {
		$survey = db_row('select * from surveys where survey_id=?', $sid);
        if ($survey['survey_type'] == 'multiple-choice-exam') {
            $questions = db_multirow('select * from survey_questions where survey_id=?', $sid);
            foreach ($questions as $q) {
                    $options = unserialize($q['question_type_extra']);
                    $qid = 'q_'.$q['survey_question_id'];
                    if ($_POST[$qid] != $options['right_answer']) {
                        form_error($qid, _('Incorrect answer.'));
                    }
            }
        }
}

function survey_handle_submission($sid) {
		$survey = db_row('select * from surveys where survey_id=?', $sid);
		$questions = db_multirow('select * from survey_questions where survey_id=?', $sid);

		$qid = $questions[0]['survey_question_id'];
		if ($survey['survey_type']=='popquiz') {
			# make sure they answered
			if (!isset($_POST['q_'.$qid])) return;
		}

		$respid = db_newid('survey_responses');
		db_send('begin');
		db_send('insert into survey_responses(survey_response_id,survey_id, respondent, respondent_ip, response_date) values (?,?,?,?,CURRENT_TIMESTAMP)',
				$respid, $sid, user_id(), $_SERVER['REMOTE_ADDR']);
		foreach ($questions as $q) {
			$qid = $q['survey_question_id'];
			db_insert_hash('survey_answers', array(
								'survey_answer_id' => db_newid('survey_answers'),
								'survey_response_id' => $respid,
								'survey_question_id' => $qid,
								'answer' => is_array($_POST['q_'.$qid]) ? serialize($_POST['q_'.$qid]) :  $_POST['q_'.$qid]
			));
		}
		db_send('commit');

		if ($survey['notify_email']) {
			$msg = sprintf("Someone filled in the %s form.  Results:\n\n", $survey['short_name']);
			i18n_get_batch($questions, 'Survey question', 'survey_question_id');
			foreach ($questions as $q) {
                $value = $_POST['q_'.$q['survey_question_id']];
                $menu = unserialize($q['question_type_extra']);
                if (is_array($menu)) {
                    if (is_array($value)) {
                        $nv = array();
                        foreach ($value as $v) {
                            $nv[] = $menu['en'][$v];
                        }
                        $value = $nv;
                    } else {
                        $value = $menu['en'][$value];
                    }
                }
                if (is_array($value)) {
                    $value = var_export($value, true);
                }

				$msg .= sprintf("%s:\n\t%s\n\n", $q['question'], $value);
			}
			cpf_mail( $survey['notify_email'], $survey['short_name'].' form filled in', $msg);
		}
}

function survey_get_summary_results($sid) {
		$responses = db_multirow('select * from survey_responses join survey_answers using (survey_response_id) join survey_questions using (survey_question_id) where survey_responses.survey_id=?', $sid);
		i18n_get_batch($responses, 'Survey question', 'survey_question_id');
		foreach ($responses as $r) {
			$qid = $r['survey_question_id'];

			if (!$rc[$qid]) {
				$rc[$qid]['question'] = $r['question'];
				$rc[$qid]['question_type'] = $r['question_type'];
				if ($r['question_type']=='menu' || $r['question_type']=='flaglist') {
                    $menu = unserialize($r['question_type_extra']);
					$options[$qid] = $menu[CPF_LANGUAGE];
				}
			}
			if ($r['question_type']=='menu' || $r['question_type']=='flaglist') {
                $answer = unserialize($r['answer']);
                if ($answer === false) {
                    $answer = array($r['answer']);
                }
                foreach ($answer as $idx) {
                    $rc[$qid][histogram][ $options[$qid][ $idx ] ]++;
                }
			} else {
				$rc[$qid][histogram][ $r['answer'] ]++;
			}
			$rc[$qid][total_responses] ++;
		}
		return $rc;
}

function survey_get_detailed_results($sid) {
		$responses = db_multirow("select *, date_part('epoch', response_date) as response_date_unix from survey_responses join survey_answers using (survey_response_id) join survey_questions using (survey_question_id) where survey_responses.survey_id=?", $sid);
		i18n_get_batch($responses, 'Survey question', 'survey_question_id');
        $questions = $answers = array();
		foreach ($responses as $r) {
			$qid = $r['survey_question_id'];

			if (!$questions[$qid]) {
				$questions[$qid]['question'] = $r['question'];
				$questions[$qid]['question_type'] = $r['question_type'];
				IF ($r['question_type']=='menu' || $r['question_type']=='flaglist') {
					$menu = unserialize($r['question_type_extra']);
					$options[$qid] = $menu[CPF_LANGUAGE];
				}
			}
			$rid = $r['survey_response_id'];
			if ($answers[$rid]) {
				if ($r['respondent']) {
					$answers[$rid]['respondent'] = user_name($r['respondent']).' - '.user_email($r['respondent']);
				} else {
					$answers[$rid]['respondent'] = '';
				}
				$answers[$rid]['survey_response_id'] = $r['survey_response_id'];
				$answers[$rid]['ip'] = $r['respondent_ip'];
				$answers[$rid]['date'] = date("Y-m-d&#160;H:i", $r['response_date_unix']);
			}
			if ($r['question_type']=='menu' || $r['question_type']=='flaglist') {
                $answer = unserialize($r['answer']);
                if ($answer === false) {
                    $answer = array($r['answer']);
                }
                $tmp = array();
                foreach ($answer as $idx) {
                    $tmp[] = $options[$qid][$idx];
                }
                $answers[$rid]['answers'][ $qid ] = join(' , ', $tmp);
			} else {
				$answers[$rid]['answers'][ $qid ] = $r['answer'];
			}
		}
		return array($questions, $answers);
}

 function survey_send_invite($iid) {
   $mailing_id = db_value('select invitation_mailing_id from survey_invitations join surveys using (survey_id) where invitation_id=?', $iid); 
   if ($mailing_id) {
     survey_send_newsletter($iid, $mailing_id);
   }
 }
 
 function survey_send_reminder($iid) {
   $mailing_id = db_value('select reminder_mailing_id from survey_invitations join surveys using (survey_id) where invitation_id=?', $iid); 
   if ($mailing_id) {
     survey_send_newsletter($iid, $mailing_id);
   }
 }
 
 function survey_send_newsletter($iid, $mailing_id) {
   db_send('begin');
   $user_id = db_value('select user_id from survey_invitations where invitation_id=?', $iid); 
   $newsletter_id = db_value('select newsletter_id from newsletter_mailings where newsletter_mailing_id=?', $mailing_id);
   $subscription_id = db_value('select newsletter_subscription_id from newsletter_subscriptions where user_id=? and newsletter_id=? and parameter=?', $user_id, $newsletter_id, 'survey-mailing');
 
   if (!$subscription_id) {
       db_insert_hash('newsletter_subscriptions', array(
             'user_id' => $user_id,
             'newsletter_id' => $newsletter_id,
             'parameter' => 'survey-mailing'
       ));
       $subscription_id = db_value('select newsletter_subscription_id from newsletter_subscriptions where user_id=? and newsletter_id=? and parameter=?', $user_id, $newsletter_id, 'survey-mailing');
   }
 
   db_insert_hash('newsletter_queue', array(
           'newsletter_queue_id' => newsletter_strong_id(),
           'newsletter_mailing_id' => $mailing_id,
           'newsletter_subscription_id' => $subscription_id,
           'extra_info' => $iid,
   ));
   db_send('commit');
 }
 
 


?>
