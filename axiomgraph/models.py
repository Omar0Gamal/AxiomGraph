from dataclasses import dataclass, field
from typing import Dict, Any, List

@dataclass
class Node:
    """Represents a semantic entity in the cognitive graph."""
    id: int
    label: str
    type: str
    properties: Dict[str, Any] = field(default_factory=dict)

@dataclass
class Edge:
    """Represents a directed logical relationship between two semantic entities."""
    source_id: int
    target_id: int
    relation: str
    weight: float = 1.0
    source_label: str = ""
    target_label: str = ""

@dataclass
class GraphContext:
    """The localized GraphRAG context retrieved during a query."""
    nodes: List[Node] = field(default_factory=list)
    edges: List[Edge] = field(default_factory=list)
