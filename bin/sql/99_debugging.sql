

create sequence error_id_seq;
create table errors (
	error_id integer not null primary key default nextval('error_id_seq'),
	
	tstamp timestamp default CURRENT_TIMESTAMP,

	level integer,
	message text,
	file text,
	file_line integer,
	context text
);

alter table errors add column user_id integer references users;


insert into roles (role_name) values ('error_admin');

insert into admin_areas(uri,description,role_id)
        select '/admin/errors/', 'PHP Error Log', role_id from roles where role_name='error_admin';

insert into users (first_name, last_name, email, password_md5,company)
	values ('Test', 'Admin', 'root@localhost.com','afcd531906db9e4fd74fbde6c64ac24d', 'Tiny Planet');
insert into users (first_name, last_name, email, password_md5, company)
	values ('Test', 'Alice', 'root+alice@localhost.com','afcd531906db9e4fd74fbde6c64ac24d', 'Arch Flowers');
insert into users (first_name, last_name, email, password_md5, company)
	values ('Test', 'Bob', 'root+bob@localhost.com','afcd531906db9e4fd74fbde6c64ac24d', 'Victoria''s Secret');
insert into user_role_map (user_id, role_id) select user_id,role_id from users,roles where email='root@localhost.com' and is_negative=0;


