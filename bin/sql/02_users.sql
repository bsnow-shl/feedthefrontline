CREATE SEQUENCE user_id_seq start 1000;
CREATE SEQUENCE role_id_seq start 1000;
CREATE SEQUENCE user_role_map_id_seq start 1000;

CREATE TABLE "users" (
	"user_id" integer DEFAULT nextval('user_id_seq') NOT NULL PRIMARY KEY,
	"email" text NOT NULL,

	screen_name text, --
	"password" text,
	password_md5 text,

	"first_name" text,
	"last_name" text,
	"company" text,
	"member_since" timestamp default CURRENT_TIMESTAMP,
	last_visit timestamp,

	check (not (
		password is null and password_md5 is null
	))
);
create unique index user_email on users(email);


create table user_diary (
	"user_id" integer not null references users,
	tstamp timestamp default CURRENT_TIMESTAMP,
	note text,
	extra text
);



CREATE TABLE "roles" (
	role_id integer default nextval('role_id_seq') NOT NULL PRIMARY KEY,

	role_name text unique,

	visible integer not null default 0 check (visible in (0,1)),

    -- 1 if the role withholds access to certain areas
    -- e.g. 'bozo' means "can't post"
	is_negative integer not null default 0 check (is_negative in (0,1)),

	role_parameters text not null default 'no' check (role_parameters in ('no', 'required', 'optional')),
	role_parameter_class_id integer,
	description text default ''
);

CREATE TABLE user_role_map (
	user_role_map_id integer not null primary key default nextval('user_role_map_id_seq'),
	user_id integer not null references users,
	role_id integer not null references roles,

	parameter integer,
	unique (user_id, role_id, parameter)
);

create view user_reporting_view as select * from users;

create table user_reports (
    report_id integer not null primary key,

    report_name text not null,
    report_url text not null,
    created_by integer not null references users (user_id),
    role_id integer references roles
);


insert into roles (role_name,visible,is_negative) values ('bozo',1,1);


