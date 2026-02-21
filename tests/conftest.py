"""Root conftest — wires up all fixtures for BBS E2E tests."""

from .fixtures.build import bbs_binary, mkconfig_binary, datadump_binary  # noqa: F401
from .fixtures.instance import bbs_instance, bbs_client        # noqa: F401
