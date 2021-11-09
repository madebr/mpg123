import logging
import os
import re
import smtplib
import ssl

import github
import markdown

logging.basicConfig(format='[%(asctime)s] %(message)s')
logging.root.setLevel(logging.INFO)

mailinglist_message = "The [mpg123-devel mailing list](https://sourceforge.net/p/mpg123/mailman/mpg123-devel/) has been notified of the existence of this pr."
mailinglist_label = "mailing-list-notified"

g = github.Github(os.environ["PUSH_GITHUB_TOKEN"])
user_me = g.get_user()
repo = g.get_repo(os.environ["GITHUB_REPOSITORY"])

for pr in repo.get_pulls(state="open"):
    logging.info("Checking PR #%d", pr.number)
    already_handled = False
    for label in pr.get_labels():
        if label.name == mailinglist_label:
            already_handled = True
    if already_handled:
        continue

    logging.info("PR #%d was NOT handled already", pr.number)

    mail_markdown = f"""\
A pull request by [{pr.user.login}]({pr.user.html_url}) was opened at {pr.created_at}.

Please visit [{pr.html_url}]({pr.html_url}) to give feedback and review the code.

---

{pr.body}

---

- url: {pr.html_url}
- patch: {pr.html_url}.patch
"""
    logging.info("markdown message: %s", mail_markdown)

    mail_title = "Opened GH-#{}: {} [{}]".format(pr.number, pr.title, pr.user.login)
    mail_body = markdown.markdown(mail_markdown)

    logging.info("mail title: %s", mail_title)
    logging.info("mail body: %s", mail_body)

    mail_message = f"""\
MIME-Version: 1.0
Content-type: text/html; charset=utf-8
Subject: {mail_title}

{mail_body}
"""

    sender_email = "{} <{}>".format("{} (via github PR)".format(pr.user.login), os.environ["MAIL_SENDER"])
    receiver_email = os.environ["MAIL_RECEIVER"].split(";")

    logging.info("mail from: %s", re.sub("[a-z0-9]", "x", sender_email, flags=re.I))
    logging.info("receiver to: %s", receiver_email)

    port = 465
    server = os.environ["MAIL_SERVER"]
    login = os.environ["MAIL_LOGIN"]
    password = os.environ["MAIL_PASSWORD"]

    context = ssl.create_default_context()

    with smtplib.SMTP_SSL(server, port, context=context) as server:
        server.login(login, password)
        server.sendmail(sender_email, receiver_email, mail_message)

    logging.info("Creating comment at PR#%d", pr.number)
    pr.add_to_labels(mailinglist_label)
    comment = pr.create_issue_comment(mailinglist_message)
    logging.info("Visit comment at %s", comment.html_url)

    logging.info("mail sent")
