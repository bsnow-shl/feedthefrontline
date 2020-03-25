
create table email_domains (
    email_domain_id integer not null primary key,

    domain_name varchar(100) unique not null
);


create table email_accounts (
    email_account_id integer not null primary key,

    class varchar(50) not null,  -- 'user', 'maillist', etc
    email_domain_id integer not null references email_domains,

    from_addr varchar(50) not null,
    to_addr varchar(255),

    password varchar(255),  -- null if user is not supposed to be able to POP mail
    moderator varchar(255),  -- null if user is not supposed to be able to POP mail

    UNIQUE(email_domain_id, from_addr,class)
);

insert into roles (role_name) values ('email_admin');
insert into admin_areas(uri,description,role_id) 
    select '/admin/email/', 'E-mail addresses', role_id from roles where role_name='email_admin';

