# -*- coding: utf-8 -*-
from __future__ import unicode_literals

from django.db import migrations, models


class Migration(migrations.Migration):

    dependencies = [
        ('WebConsole', '0001_initial'),
    ]

    operations = [
        migrations.AddField(
            model_name='authuser',
            name='kbe_bin_path',
            field=models.CharField(default='', help_text='kbe_bin_path', max_length=256),
        ),
        migrations.AddField(
            model_name='authuser',
            name='kbe_res_path',
            field=models.CharField(default='', help_text='kbe_res_path', max_length=256),
        ),
        migrations.AddField(
            model_name='authuser',
            name='kbe_root',
            field=models.CharField(default='', help_text='kbe_root', max_length=256),
        ),
    ]
