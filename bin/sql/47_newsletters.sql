CREATE TABLE newsletters (
    newsletter_id integer NOT NULL primary key,
    name text not null,
    
    callback_header text not null default '',
    xmit_callback text  not null default '',
    open_callback text not null default '',
    clickthrough_callback text not null default '',
    unsubscribe_callback text not null default ''
);



create sequence newsletter_subscriptions_id;

CREATE TABLE newsletter_subscriptions (
    newsletter_subscription_id integer not null primary key default nextval('newsletter_subscriptions_id'),
    user_id integer NOT NULL references users,
    newsletter_id integer NOT NULL references newsletters,
    smtp_sender_override text,
    parameter text
);
CREATE INDEX sub_uid ON newsletter_subscriptions USING btree (user_id);
CREATE INDEX sub_nid ON newsletter_subscriptions USING btree (newsletter_id);

CREATE TABLE newsletter_mailings (
    newsletter_mailing_id integer NOT NULL primary key,
    newsletter_id integer NOT NULL references newsletters,
    tstamp timestamp default CURRENT_TIMESTAMP,
    description text NOT NULL,
    "path" text NOT NULL,
    subject text NOT NULL,
    from_line text NOT NULL,
    sender_name text not null,
    sender_email text not null
);
create index nm_newsletter on newsletter_mailings(newsletter_id);


CREATE TABLE newsletter_queue (
    newsletter_queue_id character(16) NOT NULL primary key,
    newsletter_mailing_id integer NOT NULL references newsletter_mailings,

    extra_info text, -- to be parsed by the callbacks for the survey

    sent integer DEFAULT 0,
    opened integer DEFAULT 0,
    clicked_through integer DEFAULT 0,
    bounced integer DEFAULT 0,
    unsubscribed integer DEFAULT 0,
    newsletter_subscription_id integer NOT NULL
);
CREATE INDEX nq_mailing ON newsletter_queue USING btree (newsletter_mailing_id);

CREATE TABLE newsletter_clickthroughs (
    newsletter_queue_id character(32) references newsletter_queue,
    url text
);
create index nc_queue on newsletter_clickthroughs(newsletter_queue_id);

	
insert into roles (role_name) values ('newsletter_admin');
insert into roles (role_name) values ('newsletter_sender');
insert into admin_areas(uri,role_id,description)
        select '/admin/newsletter/',  role_id, 'Newsletters' from roles where role_name='newsletter_admin';



-- integerate other systems with newsletters
alter table surveys add column invitation_mailing_id integer references newsletter_mailings(newsletter_mailing_id);
alter table surveys add column reminder_mailing_id integer references newsletter_mailings(newsletter_mailing_id);

