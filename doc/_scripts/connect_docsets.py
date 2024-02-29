#!/usr/bin/env python3

import os
import re

pattern = r"class=\"reference external\" href=\"(.*?)\" title=\"\(in (.*?)\)\""


replacements = {
    'nrfxlib' : ("../nrfxlib/", "../../../../../nrfxlib.docset/Contents/Resources/Documents/"),
    'MCUboot' : ("../mcuboot/", "../../../../../mcuboot.docset/Contents/Resources/Documents/"),
    'Matter SDK' : ("../matter/", "../../../../../matter.docset/Contents/Resources/Documents/"),
    'nrfx' : ("../nrfx/", "../../../../../nrfx.docset/Contents/Resources/Documents/"),
    'Kconfig reference' : ("../kconfig/", "../../../../../kconfig.docset/Contents/Resources/Documents/"),
    'nRF Connect SDK' : ("../nrf/", "../../../../../nrf.docset/Contents/Resources/Documents/"),
    'Zephyr Project' : ("../zephyr/", "../../../../../zephyr.docset/Contents/Resources/Documents/"),
    'Trusted Firmware-M': ("../tfm/", "../../../../../tfm.docset/Contents/Resources/Documents/"),
}

def fix_hrefs(directory, dry_run):
    for root, dirs, files in os.walk(directory):
        for file in files:
            if file.endswith(".html"):
                file_path = os.path.join(root, file)
                with open(file_path, "r", encoding="utf-8") as f:
                    content = f.read()

                new_content = content

                found_pattern = False

                for match in re.finditer(pattern, content):
                    domain = match.group(2)
                    old_href = match.group(1)
                    for domain_prefix, replacement in replacements.items():
                        if domain.startswith(domain_prefix):
                            new_href = old_href.replace(replacement[0], replacement[1])
                            found_pattern = True
                            break
                    new_content = new_content.replace(old_href, new_href)

                if found_pattern:
                    print(f"Modifying: {file_path}")
                    if not dry_run:
                        with open(file_path, 'w', encoding='utf-8') as f:
                            f.write(new_content)
                else:
                    print(f"Skipping: {file_path}")


fix_hrefs(".", dry_run=False)
