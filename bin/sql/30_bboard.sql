CREATE TABLE bboards (
	bboard_id integer not null primary key,
	
	pretty_name text not null,
	short_name varchar(50) not null
	 check (short_name not like '% %'),
	blurb text,
	
	-- number of days that makes a thread old
	definition_of_old integer default 30,
	
	restrict_to_permission text references roles(role_name),
	restrict_to_special text,

	whatsnew_id integer references whatsnew
);
create unique index bboard_short_name on bboards(short_name);



CREATE TABLE bboard_messages (
	bboard_message_id integer not null primary key,

	bboard_id integer not null references bboards,
	parent_post integer references bboard_messages,

	post_date timestamp default CURRENT_TIMESTAMP,
	author integer not null references users(user_id),
	headline text not null,
	body text not null,

    approved boolean not null default false,

    upload_id integer references uploads (upload_id)
);

create table bboard_subscriber_map (
	bboard_subscription_id integer not null primary key,

	bboard_id integer not null references bboards,
	user_id integer not null references users,

	subscribe_posts integer not null default 0 check (subscribe_posts in (0,1)),
	subscribe_comments integer not null default 0 check (subscribe_comments in (0,1)),

	UNIQUE(bboard_id, user_id)
);

insert into roles (role_name) values ('bboard_admin');
insert into roles (role_name) values ('bboard_bozo');
insert into roles (role_name) values ('bboard_attachments');

insert into admin_areas(uri,role_id,description) 
	values ('/admin/bboard/',  (select role_id from roles where role_name='bboard_admin'), 'Bulletin Board');

