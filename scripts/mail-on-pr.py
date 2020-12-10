import logging
import os
import re
import smtplib
import ssl

import github
import markdown

logging.basicConfig(format='[%(asctime)s] %(message)s')
logging.root.setLevel(logging.INFO)

pr_id = int(re.match("refs/pull/([0-9]+)/merge", os.environ["GITHUB_REF"]).group(1))
logging.info("pull id: %d", pr_id)

g = github.Github(os.environ["PUSH_GITHUB_TOKEN"])
repo = g.get_repo(os.environ["GITHUB_REPOSITORY"])
issue = repo.get_issue(pr_id)

mail_markdown = f"""\
A pull request from {issue.user.login} was opened at {issue.created_at}.

Visit [{issue.html_url}]({issue.html_url}) to give feedback and review the code.

---

{markdown.markdown(issue.body)}

---

- url: {issue.html_url}
- patch: {issue.html_url}.patch
"""
logging.info("markdown message: %s", mail_markdown)

mail_title = "Opened GH-#{}: {} [{}]".format(issue.id, issue.title, issue.user.login)
mail_body = markdown.markdown(mail_markdown)

logging.info("mail title: %s", mail_title)
logging.info("mail body: %s", mail_body)

mail_message = f"""\
MIME-Version: 1.0
Content-type: text/html; charset=utf-8
Subject: {mail_title}

{mail_body}
"""

sender_email = "{} <{}>".format("GitHub User {}".format(issue.user.login), os.environ["MAIL_SENDER"])
receiver_email = "contact list <{}>".format(os.environ["MAIL_RECEIVER"])

port = 465
server = os.environ["MAIL_SERVER"]
login = os.environ["MAIL_LOGIN"]
password = os.environ["MAIL_PASSWORD"]

context = ssl.create_default_context()

with smtplib.SMTP_SSL(server, port, context=context) as server:
    server.login(login, password)
    server.sendmail(sender_email, receiver_email, mail_message)

print("mail sent")
