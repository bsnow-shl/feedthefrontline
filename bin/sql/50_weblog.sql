create table weblogs (
	weblog_id integer not null primary key,

	uri text not null,  -- for url's
	weblog_title text not null,
	weblog_subtitle text,
	blurb text,

	owner integer not null references users(user_id)
);
create index weblog_uri on weblogs(uri);

create table weblog_entries (
	weblog_entry_id integer not null primary key,

	weblog_id integer not null references weblogs,
	author integer not null references users(user_id),

	posting_date timestamp default(CURRENT_TIMESTAMP),
	uri text not null,
	entry_title text not null,
	body text,

	views integer not null default 0
);

create table weblog_entry_attachments (
    weblog_entry_id integer not null references weblog_entries,
    upload_id integer not null references uploads
);


create table weblog_entry_keywords (
        weblog_entry_id integer references weblog_entries,
	keyword text
);
create index weblog_entry_keywords_kw on weblog_entry_keywords(keyword);
create index weblog_entry_keywords_id on weblog_entry_keywords(weblog_entry_id);


-- integrate into comments
-- insert into comment_systems (comment_system_id, system_name, singular, plural, table_name, pkey_column) values
--	(1, 'Weblog Comments', 'weblog entry', 'weblog entries', 'weblog_entries', 'weblog_entry_id');


-- integrate into admin menu
insert into classes(class_id, class_name,table_name,table_pkey_column,table_pretty_column) 
	values (50, 'Weblogs','weblogs','weblog_id', 'weblog_title');

insert into roles (role_name) values ('weblog_admin');
insert into roles (role_name,role_parameters,role_parameter_class_id) 
	select 'weblog_editor','required',class_id from classes where class_name='Weblogs';
insert into admin_areas(uri,description,role_id) 
        select '/admin/weblog/', 'Weblogs', role_id from roles where role_name='weblog_admin';



