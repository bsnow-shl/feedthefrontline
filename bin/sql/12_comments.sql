create sequence comment_system_id start 100;

create table comment_systems  (
	comment_system_id integer not null primary key default nextval('comment_system_id'),
	
	table_name text unique not null,

	singular text not null,
	plural text not null,

	pkey_column text not null,
    title_column text not null,

	registration_required integer check (registration_required in (0,1)) default 0
);


create table comments (
	comment_system_id integer not null references comment_systems,
	commented_thing integer not null,
	comment_timestamp timestamp default CURRENT_TIMESTAMP,

	-- only guaranteed set if comment_systems.registertation_required
	user_id integer references users,

	-- otherwise, it falls back to this.
	author_name varchar(50),
	author_email varchar(50),

	comment text
);
create index comments_finder on comments(comment_system_id, commented_thing);
create index comments_system on comments(comment_system_id);
create index comments_date on comments(comment_timestamp);
create index comments_author on comments(user_id);


insert into roles (role_name) values ('comments_admin');
insert into admin_areas(uri,role_id,description) 
        select '/admin/comments/', role_id, 'Comments' from roles where role_name='comments_admin';

