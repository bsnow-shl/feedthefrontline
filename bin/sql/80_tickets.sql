
create table ticket_components (
    ticket_component_id integer not null primary key,
    active integer not null default 1,
	component varchar(50)  not null,
    responsible integer not null references users(user_id),
    unique(component)
);

create table ticket_component_cc (
    ticket_component_id integer not null references ticket_components,
	email varchar(50),
	unique (ticket_component_id,email)
);


create table tickets (
    ticket_id integer not null primary key,

    date_added timestamp default CURRENT_TIMESTAMP,
    ticket_component_id integer not null references ticket_components,

    submitter_name text not null,
    submitter_email text,
    assigned_to integer not null references users(user_id),

    short_title text not null,
    description text not null,
    ticket_status varchar(30) not null default 'open'
);


insert into roles (role_name) values ('tickets_admin');
insert into admin_areas(uri,description,role_id) 
    select '/admin/tickets/', 'Tickets', role_id from roles where role_name='tickets_admin';

INSERT INTO comment_systems (table_name, singular, plural, pkey_column, title_column, registration_required) VALUES ( 'tickets', 'ticket', 'tickets', 'ticket_id', 'ticket_id', 1);

