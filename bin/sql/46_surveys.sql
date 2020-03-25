

create table surveys (
	survey_id integer not null primary key,
	created_by integer not null references users,
	
	short_name text not null,

	survey_type text not null, -- 'popquiz' 'standard', 'multi-page'
	answers_available_to text not null, -- 'public' 'respondents' 'admin'

	survey_status text not null, -- 'draft' 'promo' 'live' 'retired'
    notify_header text,
    notify_function text,
	notify_email text

    check (survey_type in ('popquiz', 'standard', 'multiple-choice-exam', 'multi-page'))
);

create table survey_pages (
  page_id integer not null primary key,
  survey_id integer not null references surveys,

  display_order integer not null default 0,
  unique (survey_id, display_order),

  class_id integer not null references i18n_classes
);
create index sp_survey on survey_pages(survey_id);


create table survey_questions (
	survey_question_id integer not null primary key,
	survey_id integer references surveys,
	
	display_order integer,

	question_type text not null, -- 'text', 'integer','bigtext', 'menu', 'radio'
	required integer not null default 0,
	question_type_extra text
);
create index survey_q_idx on survey_questions(survey_id);


create table survey_responses (
	survey_response_id integer not null primary key,

	survey_id integer references surveys,

	respondent integer references users,
	respondent_ip varchar(25),
	response_date timestamp default CURRENT_TIMESTAMP
);
create index survey_r_idx on survey_responses(survey_id);

create table survey_answers ( 
	survey_answer_id integer not null primary key,
	survey_response_id integer not null references survey_responses,
	survey_question_id integer not null references survey_questions,

	answer text
);

create table survey_invitations (
  invitation_id integer not null primary key,
  survey_id integer not null references surveys,
  user_id integer not null references users,
  
  created_by integer not null references users(user_id),
  created_on timestamp not null default CURRENT_TIMESTAMP,
  valid_until timestamp not null,

  invitation_status text not null default 'sent',
  check (invitation_status in (
    'unsent', 'sent', 'opened', 'started', 'finished', 'void'))
);
create index si_user on survey_invitations(user_id);
create index si_survey on survey_invitations(survey_id);

create table survey_invitation_responses (
    response_id integer not null primary key,
    invitation_id integer not null references survey_invitations,
    page_id integer not null references survey_pages,
    created_by integer not null references users (user_id),
    create_tstamp timestamp not null default CURRENT_TIMESTAMP
);
create index sir_invitation on survey_invitation_responses(invitation_id);
create index sir_page on survey_invitation_responses(page_id);


create table survey_invitation_diary (
  invitation_id integer not null references survey_invitations,
  create_tstamp timestamp not null default CURRENT_TIMESTAMP,
  user_id integer references users,
  note text
);
create index sid_invitation on survey_invitation_diary(invitation_id);


insert into roles (role_name) values ('survey_admin');

insert into admin_areas(uri,role_id,description)
        values ('/admin/surveys/',  (select role_id from roles where role_name='survey_admin'), 'Surveys & forms');
	
insert into i18n_classes (class_id,class_name) values( 461,'Survey preamble');
insert into i18n_classes (class_id,class_name) values( 462,'Survey question');
insert into i18n_classes (class_id,class_name) values( 463,'Survey question options');
insert into i18n_fields (field_id, class_id,precedence, field_name, pretty_name_en, field_type) values (4611, 461, 90, 'title','Page title','text');
insert into i18n_fields (field_id, class_id,precedence, field_name, pretty_name_en, field_type) values (4612, 461, 88, 'body','Survey preameble','richtext');
insert into i18n_fields (field_id, class_id,precedence, field_name, pretty_name_en, field_type) values (4613, 461, 96, 'thank_you_message','Thank-you message','richtext');
insert into i18n_fields (field_id, class_id,precedence, field_name, pretty_name_en, field_type) values (4614, 461, 98, 'form','Survey template','richtext');
insert into i18n_fields (field_id, class_id,precedence, field_name, pretty_name_en, field_type) values (4621, 462, 92, 'question','question','text');
insert into i18n_fields (field_id, class_id,precedence, field_name, pretty_name_en, field_type) values (4631, 463, 94, 'option','option','text');

